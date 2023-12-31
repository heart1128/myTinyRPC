#include "tinypb_rpc_channel.h"

#include <memory>
#include <google/protobuf/service.h>
#include <google/protobuf/message.h>
#include <google/protobuf/descriptor.h>
#include "src/net/net_address.h"
#include "src/comm/error_code.h"
#include "src/net/tcp/tcp_client.h"
#include "src/net/tinypb/tinypb_rpc_channel.h"
#include "src/net/tinypb/tinypb_rpc_controller.h"
#include "src/net/tinypb/tinypb_codec.h"
#include "src/net/tinypb/tinypb_data.h"
#include "src/comm/log.h"
#include "src/comm/msg_req.h"
#include "src/comm/run_time.h"

namespace tinyrpc{


TinyPbRpcChannel::TinyPbRpcChannel(NetAddress::ptr addr)
: m_addr(addr)
{
}

void TinyPbRpcChannel::CallMethod(const google::protobuf::MethodDescriptor *method, 
                google::protobuf::RpcController *controller, 
                const google::protobuf::Message *request, 
                google::protobuf::Message *response, 
                google::protobuf::Closure *done)
{
    TinyPbStruct pb_struct;
    
    TinyPbRpcController* rpc_controller = dynamic_cast<TinyPbRpcController*>(controller);
    if(!rpc_controller)
    {
        ErrorLog << "call failed. falid to dynamic cast TinyPbRpcController";
        return;
    }

    // 设置客户端信息
    TcpClient::ptr m_client = std::make_shared<TcpClient>(m_addr);
    rpc_controller->SetLocalAddr(m_client->getLocalAddr());
    rpc_controller->SetPeerAddr(m_client->getPeerAddr());

    // 设置服务全名
    pb_struct.service_full_name = method->full_name();
    DebugLog << "call service_name = " << pb_struct.service_full_name;

    // 序列化客户端要发送的包
    // 内容就是proto定义的request的请求函数字段，就是要调用那个message，然后服务器处理填充这些字段进行返回
    if(!request->SerializeToString(&pb_struct.pb_data))
    {
        ErrorLog << "serialize send package error";
        return;
    }

    // 消息序列是空的就随机生成
    if(!rpc_controller->MsgSeq().empty())
    {
        pb_struct.msg_req = rpc_controller->MsgSeq();
    }
    else
    {
        // 当前协程的msgno作为新序列
        RunTime* run_time = getCurrentRunTime();
        if(run_time != NULL && !run_time->m_msg_no.empty())
        {
            pb_struct.msg_req = run_time->m_msg_no;
            DebugLog << "get from RunTime succ, msgno = " << pb_struct.msg_req;
        }
        else // 当前协程也没有，随机生成
        {
            pb_struct.msg_req = MsgReqUtil::genMsgNumber();
            DebugLog << "get from RunTime error, generate new msgno = " << pb_struct.msg_req;
        }
        rpc_controller->SetMsgReq(pb_struct.msg_req);
    }

    // requset的序列化消息进行编码成字节流
    AbstractCodeC::ptr m_codec = m_client->getConnection()->getCodec();
    m_codec->encode(m_client->getConnection()->getOutBuffer(), &pb_struct); // 编码发送请求，也就是编码到m_write_buffer中

    // 结构体自带一个判断编码成功的标志
    if(!pb_struct.encode_succ)
    {
        rpc_controller->SetError(ERROR_FAILED_ENCODE, "encode tinypb data error");
        return;
    }

    InfoLog << "============================================================";
    InfoLog << pb_struct.msg_req << "|" << rpc_controller->PeerAddr()->toString() 
        << "|. Set client send request data:" << request->ShortDebugString();
    InfoLog << "============================================================";
    m_client->setTimeout(rpc_controller->Timeout());


    /////////////////////
    // 创建response数据结构体，这个结构体不是要发送的数据，是接收服务端发送的数据，所以称为response
    TinyPbStruct::pb_ptr res_data;
    int rt = m_client->sendAndRecvTinyPb(pb_struct.msg_req, res_data); // 发送远程调用，接收回复的包，res_data就是接受了回复包的信息
    if (rt != 0) 
    {
        rpc_controller->SetError(rt, m_client->getErrInfo());
        ErrorLog << pb_struct.msg_req << "|call rpc occur client error, service_full_name=" << pb_struct.service_full_name << ", error_code=" 
            << rt << ", error_info = " << m_client->getErrInfo();
        return;
    }

    // 拿到回复的数据包，进行字符串->proto的反序列化解析，保存到response中
    if(!response->ParseFromString(res_data->pb_data))
    {
        rpc_controller->SetError(ERROR_FAILED_DESERIALIZE, "failed to deserialize data from server");
        ErrorLog << pb_struct.msg_req << "|failed to deserialize data";
        return;
    }
    // 查看返回包的错误信息
    if (res_data->err_code != 0) 
    {
        ErrorLog << pb_struct.msg_req << "|server reply error_code=" << res_data->err_code << ", err_info=" << res_data->err_info;
        rpc_controller->SetError(res_data->err_code, res_data->err_info);
        return;
    }

    InfoLog<< "============================================================";
    InfoLog<< pb_struct.msg_req << "|" << rpc_controller->PeerAddr()->toString()
        << "|call rpc server [" << pb_struct.service_full_name << "] succ" 
        << ". Get server reply response data:" << response->ShortDebugString();
    InfoLog<< "============================================================";

    // 执行最后的回调函数
    if(done)
        done->Run();

}

    
} // namespace tinyrpc

