// 通道实现交互，需要实现callMethod，作用是转换requset到response的数据，序列化和反序列化

#ifndef SRC_NET_TINYPB_TINYPB_RPC_CHANNEL_H
#define SRC_NET_TINYPB_TINYPB_RPC_CHANNEL_H 

#include <memory>
#include <google/protobuf/service.h>
#include "src/net/net_address.h"
#include "src/net//tcp/tcp_client.h"

namespace tinyrpc {

class TinyPbRpcChannel : public google::protobuf::RpcChannel{

public:
    typedef std::shared_ptr<TinyPbRpcChannel> ptr;

public:
    TinyPbRpcChannel(NetAddress::ptr addr);
    ~TinyPbRpcChannel() = default;

    void CallMethod(const google::protobuf::MethodDescriptor* method, 
    google::protobuf::RpcController* controller, 
    const google::protobuf::Message* request, 
    google::protobuf::Message* response, 
    google::protobuf::Closure* done) override;

private:
    NetAddress::ptr m_addr;

};
    
} // namespace tinyrpc



#endif
