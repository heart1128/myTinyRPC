// 编码过程就是将TinyPbStruct以字节流形式写入buf，注意数字在本地和网络流需要大小端转换
// 解码过程就是相反，同样需要转换
#include <vector>
#include <algorithm>
#include <sstream>
#include <memory>
#include <cstring>
#include <arpa/inet.h> // htonl
#include "src/net/tinypb/tinypb_codec.h"
#include "src/comm/log.h"
#include "src/net/abstract_data.h"
#include "src/net/tinypb/tinypb_data.h"
#include "tinypb_codec.h"
#include "src/comm/msg_req.h"
#include <iostream>

namespace tinyrpc{

static const char PB_START = 0x02; // 协议包开始标志
static const char PB_END = 0x03; // 结束标志
static const int MSG_REQ_LEN = 20; // request和respone的默认长度

TinyPbCodeC::TinyPbCodeC()
{

}

TinyPbCodeC::~TinyPbCodeC()
{

}

void TinyPbCodeC::encode(TcpBuffer *buf, AbstractData *data)
{
    if(!buf || !data)
    {
        ErrorLog << "encode error! buf or data nullptr";
        return;
    }
    
    // 动态转换为子类，父类到子类的转换才会触发动态检查机制
    TinyPbStruct* tmp = dynamic_cast<TinyPbStruct*>(data);
    

    int len = 0;
    const char* re = encodePbData(tmp, len);

    // 1. 编码错误，没有编码长度
    if(re == nullptr || len == 0 || !tmp->encode_succ)
    {
        ErrorLog << "encode error!";
        data->encode_succ = false; // 设置编码状态标志
        return;
    }
    // 2. 编码成功
    DebugLog << "encode package len = " << len;
    if(re != nullptr)
    {
        buf->writeToBuffer(re, len); // 编码成功加入到需要发送也就是写buffer
        DebugLog << "succ encode and write to buffer, writeindex=" << buf->writeIndex();
    }
    data = tmp;  //data转换回去
    // 释放空间
    if(re)
    {
        free((void*)re);  // 全部以void*作为中间类型
        re = NULL;
    }
}

// 进行PB协议数据的编码，TinyPbStruct -> net bytes
const char *TinyPbCodeC::encodePbData(TinyPbStruct *data, int &len)
{
    // 1. 服务名判断。服务名错误整个服务调用就错误了
    if(data->service_full_name.empty())
    {
        ErrorLog << "parse error, service_full_name is empty!";
        data->encode_succ = false;
        return nullptr;
    }

    // 2.请求或者回应消息体为空，生成随机序列填补，可以没有请求消息 
    if(data->msg_req.empty())
    {
        data->msg_req = MsgReqUtil::genMsgNumber(); // 利用/dev/产生随机数填补
        data->msg_req_len = data->msg_req.length();
        DebugLog << "generate msgno = " << data->msg_req;
    }

    // 3.写入结构体中对应的各项数据到一个buf中。
        //3.1 总协议报文包长度,其中两个char是开头和结尾的两个字符长度
    int32_t pk_len = 2 * sizeof(char) + 6 * sizeof(int32_t)
                    + data->pb_data.length() + data->service_full_name.length()
                    + data->err_info.length() + data->pb_data.length();

    DebugLog << "encoder package len = " << pk_len;
        // 3.2 使用一个tmp字节指针指向buf，字节复制
    char* buf = reinterpret_cast<char*>(malloc(pk_len)); // 字节端转换
    char* tmp = buf;
        // 3.3 加入开头标志
    *tmp = PB_START;
    ++tmp;
        // 3.4 加入总长度，注意的是要将数字转换为网络字节流小端
    int32_t pk_len_net = htonl(pk_len);
    memcpy(tmp, &pk_len_net, sizeof(int32_t));
    tmp += sizeof(int32_t);
        // 3.5 加入msg_req
    int32_t msg_req_len = data->msg_req.length();
    DebugLog << "msg_req_len = " << msg_req_len;
    int32_t msg_req_len_net = htonl(msg_req_len);
    memcpy(tmp, &msg_req_len_net, sizeof(int32_t));
    tmp += sizeof(int32_t);
    if(msg_req_len != 0) // 有消息才写入
    {
        memcpy(tmp, &data->msg_req[0], msg_req_len);
        tmp += msg_req_len;
    }
        // 3.6 加入完整服务名
    int32_t service_full_name_len = data->service_full_name.length();
    int32_t service_full_name_len_net = htonl(service_full_name_len);
    DebugLog << "service_name_len = " << service_full_name_len;
    memcpy(tmp, &service_full_name_len_net, sizeof(int32_t));
    tmp += sizeof(int32_t);
    if(service_full_name_len != 0)
    {
        memcpy(tmp, &data->service_full_name[0], service_full_name_len);
        tmp += service_full_name_len;
    }
        //3.7 加入框架级错误代码
    int32_t err_code_net = htonl(data->err_code);
    memcpy(tmp, &err_code_net, sizeof(int32_t));
    tmp += sizeof(int32_t);

        // 3.8加入错误信息
    int32_t err_info_len = data->err_info.length();
    DebugLog << "err_info_len = " << err_info_len;
    int32_t err_info_len_net = htonl(err_info_len);
    memcpy(tmp, &err_info_len_net, sizeof(int32_t));
    tmp += sizeof(int32_t);
    if(err_info_len != 0)
    {
        memcpy(tmp, &data->err_info[0], err_info_len);
        tmp+= err_info_len;
    }

        // 3.9 加入pb_data
    int32_t pb_data_len = data->pb_data.length();
    DebugLog << "pb_data_len = " << pb_data_len;
    memcpy(tmp, &data->pb_data[0], pb_data_len);
    tmp += pb_data_len;

        //3.10 加入检验和，这里只是考虑了检验和，没有实现
    int32_t checksum = 1;
    int32_t checksum_net = htonl(checksum);
    memcpy(tmp, &checksum_net, sizeof(int32_t));
    tmp += sizeof(int32_t);
    
        // 3.11加入结束符
    *tmp = PB_END;


    data->pk_len = pk_len;
    data->msg_req_len = msg_req_len;
    data->service_name_len = service_full_name_len;
    data->err_info_len = err_info_len;

    data->check_num = checksum;
    data->encode_succ = true;

    len = pk_len;

    return buf;
}

int32_t getInt32FromNetByte(const char* buf)
{
    int32_t tmp;
    memcpy(&tmp, buf, sizeof(int32_t));
    return ntohl(tmp);
}

// 对编码的字节流进行解码
void TinyPbCodeC::decode(TcpBuffer *buf, AbstractData *data)
{
    if(!buf || !data)
    {
        ErrorLog << "decode erroe! buf or data nullptr!";
        data->decode_succ = false;
        return;
    }

    // 获取读取到的buf内容
    std::vector<char> tmp = buf->getBufferVector();
    // buf不止读取一次服务内容，所以从可读位置读取
    int start_index = buf->readIndex();
    int end_index = -1;
    int32_t pk_len = -1;

    bool parse_full_pack = false;

    // 检查开头标志和结尾标志，没有开头标志，或者有开头没结尾的缓冲区溢出问题
    for(int i = 0; i < buf->writeIndex(); ++i)
    {
        // 开头标志
        if(tmp[i] == PB_START)
        {
            // 判断溢出
            if(i + 1 < buf->writeIndex())
            {
                // 第一个字段是包大小
                pk_len = getInt32FromNetByte(&tmp[i+1]);
                DebugLog << "prase pk_len =" << pk_len;
                // 计算整个包的最后位置，判断溢出
                int j = i + pk_len - 1;
                DebugLog << "package end = " << j << "package start = " << i;

                if(j >= buf->writeIndex())  // 超出了，表示没有读完，跳过继续读写
                {
                    continue;
                }

                // 没有超出，判断结尾是否接收了整个包
                if(tmp[j] == PB_END)
                {
                    start_index = i;
                    end_index = j;
                    DebugLog << "package recrive correct!";
                    parse_full_pack = true;
                    break;
                }
            }
        }
    }

    if (!parse_full_pack) 
    {
        DebugLog << "not parse full package, return";
        return;
    }

    // 开始解析包
    buf->recycleRead(end_index + 1 - start_index); // 将read_index之前的空白消除，往前滚动
    DebugLog << "m_read_buffer size=" << buf->getBufferVector().size() << "readIndex=" << buf->readIndex() << "writeIndex=" << buf->writeIndex();

    // 创建结构体作为解析标准格式载体
    TinyPbStruct* pb_struct = dynamic_cast<TinyPbStruct*>(data);

    pb_struct->decode_succ = false;// 最后完成再设置回来
    pb_struct->pk_len = pk_len;  // 包大小
        // msg_req_len
    int msg_req_len_index = start_index + sizeof(char) + sizeof(int32_t); // msg_req_len位置，开头去掉开始标志char，和包大小数据长度int32_t
    DebugLog << "msg_req_len_index= " << msg_req_len_index;
    if(msg_req_len_index >= end_index)
    {
        ErrorLog << "parse error, msg_req_len_index[" << msg_req_len_index << "] >= end_index[" << end_index << "]";
        return;
    }
    pb_struct->msg_req_len = getInt32FromNetByte(&tmp[msg_req_len_index]);
    if(pb_struct->msg_req_len == 0)
    {
        ErrorLog << "prase error, msg_req empty！";
        return;
    }
    DebugLog << "msg_req_len= " << pb_struct->msg_req_len;

        //msg_req
    char msg_req[50] = {0};// 此时结构体内的string还没有创建，要使用一个临时对象
    int msg_req_index = msg_req_len_index + sizeof(int32_t);
    DebugLog << "msg_req_index= " << msg_req_index;
    memcpy(&msg_req[0], &tmp[msg_req_index], pb_struct->msg_req_len); 
    pb_struct->msg_req = std::string(msg_req);    

        // service_name_len
    int service_name_len_index = msg_req_index + pb_struct->msg_req_len;
    if (service_name_len_index >= end_index) 
    {
        ErrorLog << "parse error, service_name_len_index[" << service_name_len_index << "] >= end_index[" << end_index << "]";
        // drop this error package
        return;
    }
        
        // service_name
    DebugLog << "service_name_len_index = " << service_name_len_index;
    int service_name_index = service_name_len_index + sizeof(int32_t);

    if (service_name_index >= end_index) 
    {
        ErrorLog << "parse error, service_name_index[" << service_name_index << "] >= end_index[" << end_index << "]";
        return;
    }

    pb_struct->service_name_len = getInt32FromNetByte(&tmp[service_name_len_index]);

    if (pb_struct->service_name_len > pk_len) 
    {
        ErrorLog << "parse error, service_name_len[" << pb_struct->service_name_len << "] >= pk_len [" << pk_len << "]";
        return;
    }
    DebugLog << "service_name_len = " << pb_struct->service_name_len;

    char service_name[512] = {0};

    memcpy(&service_name[0], &tmp[service_name_index], pb_struct->service_name_len);
    pb_struct->service_full_name = std::string(service_name);
    DebugLog << "service_name = " << pb_struct->service_full_name;

    int err_code_index = service_name_index + pb_struct->service_name_len;
    pb_struct->err_code = getInt32FromNetByte(&tmp[err_code_index]);

    int err_info_len_index = err_code_index + sizeof(int32_t);

    if (err_info_len_index >= end_index) 
    {
        ErrorLog << "parse error, err_info_len_index[" << err_info_len_index << "] >= end_index[" << end_index << "]";
        // drop this error package
        return;
    }
    pb_struct->err_info_len = getInt32FromNetByte(&tmp[err_info_len_index]);
    DebugLog << "err_info_len = " << pb_struct->err_info_len;
    int err_info_index = err_info_len_index + sizeof(int32_t);

    char err_info[512] = {0};

    memcpy(&err_info[0], &tmp[err_info_index], pb_struct->err_info_len);
    pb_struct->err_info = std::string(err_info); 

    int pb_data_len = pb_struct->pk_len 
                        - pb_struct->service_name_len - pb_struct->msg_req_len - pb_struct->err_info_len
                        - 2 * sizeof(char) - 6 * sizeof(int32_t);

    int pb_data_index = err_info_index + pb_struct->err_info_len;
    DebugLog << "pb_data_len= " << pb_data_len << ", pb_index = " << pb_data_index;

    if (pb_data_index >= end_index) 
    {
        ErrorLog << "parse error, pb_data_index[" << pb_data_index << "] >= end_index[" << end_index << "]";
        return;
    }
    // DebugLog << "pb_data_index = " << pb_data_index << ", pb_data.length = " << pb_data_len;

    std::string pb_data_str(&tmp[pb_data_index], pb_data_len);
    pb_struct->pb_data = pb_data_str;

    // DebugLog << "decode succ,  pk_len = " << pk_len << ", service_name = " << pb_struct->service_full_name; 

    pb_struct->decode_succ = true;
    data = pb_struct;

}

ProtocalType TinyPbCodeC::getProtocalType()
{
    return TinyPb_Protocal;
}

}
