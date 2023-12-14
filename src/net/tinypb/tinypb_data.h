// 定义了pb协议报文结构体，编解码就是结构类型的转换

#ifndef SRC_NET_TINYPB_TINYPB_DATA_H
#define SRC_NET_TINYPB_TINYPB_DATA_H

#include <stdint.h> // 定义了一些固定宽度的整数类型别名
#include <vector>
#include <string>
#include "src/net/abstract_data.h"
#include "src/comm/log.h"

namespace tinyrpc{

class TinyPBStruct : public AbstractData{
public:
    typedef std::shared_ptr<TinyPBStruct> ptr;

public:
    TinyPBStruct() = default;
    ~TinyPBStruct() =default;

    TinyPBStruct(const TinyPBStruct&) = default; // 默认拷贝构造
    TinyPBStruct& operator=(const TinyPBStruct&) = default; // 默认重载=
    TinyPBStruct(TinyPBStruct&&) = default; // 默认移动构造
    TinyPBStruct& operator=(TinyPBStruct&&) = default; // 默认重载移动=

    // 结构体最小包大小为 1 + 4 + 4 + 4 + 4 + 4 + 4 + 1 = 26 bytes

    // 协议字段
    int32_t pk_len {0};     //包长度,byte

    int32_t msg_req_len {0}; // 请求或者响应字段长度
    std::string msg_req; // 表示一个rpc请求或者响应。

    int32_t service_name_len {0}; // 服务名长度
    std::string service_full_name; // 完整的rpc方法名，比如自定义了QueryServiceImpl，需要找到age，用protoc生成的有一个方法queryage,完整的方法名就是QueryServiceImpl.queryage

    int32_t err_code {0}; // 框架级错误。0调用正常。非0调用失败

    int32_t err_info_len; // 错误信息长度
    std::string err_info; // 详细的错误信息，错误才有

    std::string pb_data; // protobuf数据，protobuf序列化后得到

    int32_t check_num {0}; // 包的校验和，检验包损坏。

};

}


#endif