#include <memory>
#include <google/protobuf/service.h>     // rpc的Service基类
#include <google/protobuf/message.h>
#include <google/protobuf/descriptor.h>

#include "src/net/net_address.h"
#include "src/comm/error_code.h"    // rpc框架错误代码
#include "src/net/tcp/tcp_client.h"
#include "src/net/tinypb/tinypb_rpc_channel.h"    // rpc通道，实现callmethod, 继承google::protobuf::RpcChannel
#include "src/net/tinypb/tinypb_rpc_controller.h"  // 定义rpcService error继承google::protobuf::RpcController
#include "src/net/tinypb/tinypb_rpc_closure.h"
#include "src/net/tinypb/tinypb_codec.h"
#include "src/net/tinypb/tinypb_data.h"
#include "src/comm/log.h"
#include "src/comm/msg_req.h"    // 没有request/resopne的时候产生随机数填充
#include "src/comm/run_time.h"   // 定义了request/resopne和接口名
#include "tinypb_rpc_dispatcher.h"


namespace tinyrpc {

class TcpBuffer;

// 只有服务端才分发任务，data是客户端发送的远程调用请求
void TinyPbRpcDispacther::dispatcher(AbstractData *data, TcpConnection *conn)
{
    
    TinyPbStruct* tmp = dynamic_cast<TinyPbStruct*>(data);
    if(tmp == nullptr)
    {
        ErrorLog << "dynamic_cast<> error!";
        return;
    }

    Coroutine::getCurrentCoroutine()->getRunTime()->m_msg_no = tmp->msg_req;  // 当前协程的单例
    setCurrentRunTime(Coroutine::getCurrentCoroutine()->getRunTime());
    InfoLog << "begin to dispatch client tinypb request, msgno=" << tmp->msg_req;

    // 1. 获取完整服务名
        // 通过解析完整名称得到服务名和方法名
    std::string service_name;
    std::string method_name;

    TinyPbStruct reply_pk;
    reply_pk.service_full_name = tmp->service_full_name;
    reply_pk.msg_req = tmp->msg_req;
        // 如果request是空的，产生随机数填充
    if(reply_pk.msg_req.empty())
    {
        reply_pk.msg_req = MsgReqUtil::genMsgNumber();
    }
        // 解析得到服务名和方法名
    if(!parseServiceFullName(reply_pk.service_full_name, service_name, method_name))
    {
        ErrorLog << reply_pk.msg_req << "|parse service name " << tmp->service_full_name << "error";
        // 设置对应错误代码
        reply_pk.err_code = ERROR_PARSE_SERVICE_NAME;
        // 写入具体错误信息到包内容
        std::stringstream ss;
        ss << "cannot parse service_name:[" << reply_pk.service_full_name << "]";
        reply_pk.err_info = ss.str();
        // 错误信息也要发回客户端用于判断错误，所以编码加入TcpConnection发送
        // 
        conn->getCodec()->encode(conn->getOutBuffer(), dynamic_cast<AbstractData*>(&reply_pk));
        return;
    }
    
    // 2. 在已经注册服务名的map查找本次服务名
    Coroutine::getCurrentCoroutine()->getRunTime()->m_interface_name = tmp->service_full_name;
    auto it = m_service_map.find(service_name);
    if(it == m_service_map.end() || !((*it).second))  // 没有注册或者没有绑定google::protobuf::service
    {
        // 设置错误码
        reply_pk.err_code = ERROR_SERVICE_NOT_FOUND;
        std::stringstream ss;
        ss << "not found service_name:[" << service_name << "]";
        ErrorLog << reply_pk.msg_req << "|" << ss.str();
        reply_pk.err_info = ss.str();
        conn->getCodec()->encode(conn->getOutBuffer(), dynamic_cast<AbstractData*>(&reply_pk));

        InfoLog << "end dispatch client tinypb request, msgno=" << tmp->msg_req;
        return;
    }

    // 3. 拿出对应服务名设置的goole::probuf::service基类对象
    service_ptr service = (*it).second; // 这个service类是通过注册得到的，是通过继承proto编译之后的h文件创建的一个Queryservice创建的类，定义了自己的使用方法

        // 通过descriptorPool和Method工厂类获取MethodDescriptor类
    const google::protobuf::MethodDescriptor* method = service->GetDescriptor()->FindMethodByName(service_name);
    if(!method)
    {
        // 方法名查找失败，设置错误码
        reply_pk.err_code = ERROR_FAILED_DESERIALIZE;
        std::stringstream ss;
        ss << "not found method_name:[" << method_name << "]";
        ErrorLog << reply_pk.msg_req << "|" << ss.str();
        reply_pk.err_info = ss.str();
        conn->getCodec()->encode(conn->getOutBuffer(), dynamic_cast<AbstractData*>(&reply_pk));
        return;
    }

    // 4. 通过MethodDescriptor找到request的方法名，进行反射创建实例, Message是基类
    google::protobuf::Message* request = service->GetRequestPrototype(method).New();
    DebugLog << reply_pk.msg_req << "|request.name = " << request->GetDescriptor()->full_name();
        // 将string数据反序列化到proto中定义的数据类型数据
    if(!request->ParseFromString(tmp->pb_data))
    {
        reply_pk.err_code = ERROR_FAILED_SERIALIZE;
        std::stringstream ss;
        ss << "faild to parse request data, request.name:[" << request->GetDescriptor()->full_name() << "]";
        reply_pk.err_info = ss.str();
        ErrorLog << reply_pk.msg_req << "|" << ss.str();
        delete request;
        conn->getCodec()->encode(conn->getOutBuffer(), dynamic_cast<AbstractData*>(&reply_pk));
        return;
    }

    InfoLog << "============================================================";
    InfoLog << reply_pk.msg_req <<"|Get client request data:" << request->ShortDebugString();  // 序列化数据反解出字符串
    InfoLog << "============================================================";

    // 5. 反射创建response实例
    google::protobuf::Message* response = service->GetResponsePrototype(method).New();
    DebugLog << reply_pk.msg_req << "|response.name = " << response->GetDescriptor()->full_name();

    // 6. 为调用CallMethod准备google::protobuf::RpcControllel
    TinyPbRpcController rpc_controller;
    rpc_controller.SetMsgReq(reply_pk.msg_req);
    rpc_controller.SetMethodName(method_name);
    rpc_controller.SetMethodFullName(tmp->service_full_name);

    // 7. 定义RpcClosure, 返回需要
    // 用户把 done 保存下来，在服务回调之后的某事件发生时再调用，即实现异步 Service
    // 这个函数在调用done->Run()是被执行
        // 设置要执行的函数，这里什么都不执行，只是应付调用
    std::function<void()> reply_package_func = [](){};
    TinyPbRpcClosure closure(reply_package_func);

    // 8. 调用service->CallMethod方法，这个方法调用子类的，需要重载RpcChannel的CallMethod方法
        // 作用就是将requset的请求内容执行，然后返回信息放到response中
        // 这个service是继承了QueryService的子类QueryServiceIml，但是没有重载CallMethod
        // 所以这里调用的是QueryService的CallMethod,里面实现的是通过method的序号判断使用哪个函数
        // 之后调用这个函数，而这个函数是子类重载了的，所以调用子类的函数，达到目的
    service->CallMethod(method, &rpc_controller, request, response, &closure);
    InfoLog << "Call [" << reply_pk.service_full_name << "] succ, now send reply package";

    // 9. response结构进行序列化到String，放入reply_pk.pb_data中进行传输
        // request是反序列化string到proto
    if(!(response->SerializeToString(&(reply_pk.pb_data))))
    {
        reply_pk.pb_data = "";
        ErrorLog << reply_pk.msg_req << "|reply error! encode reply package error";
        reply_pk.err_code = ERROR_FAILED_SERIALIZE;
        reply_pk.err_info = "failed to serilize relpy data";
    }
    else
    {
        InfoLog << "============================================================";
        InfoLog << reply_pk.msg_req << "|Set server response data:" << response->ShortDebugString();
        InfoLog << "============================================================";
    }

    // 结束删除反射的实例
    delete request;
    delete response;
    
    // 进行最后的tcp发送
    conn->getCodec()->encode(conn->getOutBuffer(), dynamic_cast<AbstractData*>(&reply_pk));

}

bool TinyPbRpcDispacther::parseServiceFullName(const std::string &full_name, std::string &service_name, std::string &method_name)
{
    if(full_name.empty())
    {
        ErrorLog << "full_name is empty!";
        return false;
    } 
    //全名的格式是 service_name.method_name
    // 所以使用find找出.进行sub_str拿出来就行
    long unsigned int idx = full_name.find('.');
    if(idx == full_name.npos)
    {
        ErrorLog << "service_full_name parse error! Not char '.'！";
        return false;
    }

    service_name = full_name.substr(0, idx);
    DebugLog << "service_name = " << service_name;
    method_name = full_name.substr(idx + 1, full_name.length() - idx - 1);
    DebugLog << "method_name = " << method_name;

    return true;
}

// 使用时调用注册，就是加入到map中
void TinyPbRpcDispacther::registerService(service_ptr service)
{
    std::string service_name = service->GetDescriptor()->full_name();
    m_service_map[service_name] = service;
    InfoLog << "succ register service[" << service_name << "]!"; 
}

} // namespace tinyrpc
