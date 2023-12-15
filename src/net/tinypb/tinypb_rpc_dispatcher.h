#ifndef SRC_NET_TINYPB_TINYPB_RPC_DISPATCHER_H
#define SRC_NET_TINYPB_TINYPB_RPC_DISPATCHER_H

#include <google/protobuf/message.h>
#include <google/protobuf/service.h>
#include <google/protobuf/descriptor.h>
#include <map>
#include <memory>

#include "src/net/abstract_dispatcher.h"
#include "src/net/tinypb/tinypb_data.h"

namespace tinyrpc{

class TinyPbRpcDispacther : public AbstractDispatcher{

public:
    typedef std::shared_ptr<google::protobuf::Service> service_ptr;

public:
    TinyPbRpcDispacther() = default;
    ~TinyPbRpcDispacther() = default;

    void dispatch(AbstractData* data, TcpConnection* conn);  // 事件分发

    bool parseServiceFullName(const std::string& full_name, std::string& service_name, std::string& method_name);  // 解析服务名和方法

    void registerService(service_ptr service);  // 注册到map

public:
    // 程序开始之前，所有的服务都应该注册在这个map中
   // key: service_name
    std::map<std::string, service_ptr> m_service_map;

};

    
} // namespace tinyrp




#endif