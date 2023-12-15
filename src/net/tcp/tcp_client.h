// 客户端，进行远程调用
#ifndef SRC_NET_TCP_TCP_CLIENT_H
#define SRC_NET_TCP_TCP_CLIENT_H

#include <memory>
#include <google/protobuf/service.h>
#include "src/coroutine/coroutine.h"
#include "src/coroutine/coroutine_hook.h"
#include "src/net/net_address.h"
#include "src/net/reactor.h"
#include "src/net/tcp/tcp_connection.h"
#include "src/net/abstract_codec.h"

namespace tinyrpc{

// 客户端使用一个子协程

class TcpClient{

public:
    typedef std::shared_ptr<TcpClient> ptr;

public:
    TcpClient(NetAddress::ptr addr, ProtocalType type = TinyPb_Protocal);
    ~TcpClient();

public:
    void init();

    void resetFd();

    int sendAndRecvTinyPb(const std::string& msg_no, TinyPbStruct::pb_ptr& res);

    void stop();

    TcpConnection* getConnection(); // 返回本次使用的连接

     void setTimeout(const int v) 
     {
        m_max_timeout = v;
    }

    void setTryCounts(const int v) 
    {
        m_try_counts = v;
    }

    const std::string& getErrInfo() 
    {
        return m_err_info;
    }

    NetAddress::ptr getPeerAddr() const 
    {
        return m_peer_addr;
    }

    NetAddress::ptr getLocalAddr() const 
    {
        return m_local_addr;
    }

    AbstractCodeC::ptr getCodeC() 
    {
        return m_codec;
    }


private:
    int m_family {0};  // socket使用的协议族
    int m_fd {-1};
    int m_try_counts {3}; // 尝试重新连接的最大时间
    int m_max_timeout {10000}; // 连接超时时间
    bool m_is_stop {false};
    std::string m_err_info;  // 客户端错误信息

    NetAddress::ptr m_local_addr {nullptr}; // 本地地址
    NetAddress::ptr m_peer_addr {nullptr};
    Reactor* m_reactor {nullptr}; // 处理io的子reator
    TcpConnection::ptr m_connection {nullptr}; // 使用的连接
    AbstractCodeC::ptr m_codec {nullptr}; // 数据的编解码

    bool m_connect_suncc {false};
};

}

#endif