// 事件分发器，选择分发类型，http或者自建的tinypb

#ifndef SRC_NET_ABSTRACT_DISPATCHER_H
#define SRC_NET_ABSTRACT_DISPATCHER_H

#include <memory>
#include <google/protobuf/service.h>
#include "src/net/abstract_data.h"
#include "src/net/tcp/tcp_connection.h"

namespace tinyrpc{

class TcpConnection;

class AbstractDispatcher{
public:
    typedef std::shared_ptr<AbstractDispatcher> ptr;

public:
    AbstractDispatcher() {}

    virtual ~AbstractDispatcher() {}

    // 拿到解码的数据，然后产生respone，进行conn的output()传输出去
    virtual void dispatcher(AbstractData* data, TcpConnection* conn) = 0;

};
    
} // namespace tinyrp





#endif
