#ifndef SRC_NET_TCP_TCP_SERVER_H
#define SRC_NET_TCP_TCP_SERVER_H

#include <map>
#include <google/protobuf/service.h>
#include "src/net/reactor.h"
#include "src/net/fd_event.h"
#include "src/net/timer.h"
#include "src/net/net_address.h"
#include "src/net/abstract_codec.h"
#include "src/net/tcp/io_thread.h"
#include "src/net/tcp/tcp_connection_time_wheel.h"
#include "src/net/timer.h"

namespace tinyrpc{


// 这个类负责liten ，使用accept_hook()加入epoll监听accept
class TcpAcceptor
{
public:
    typedef std::shared_ptr<TcpAcceptor> ptr;

public:
    TcpAcceptor(NetAddress::ptr net_addr);
    ~TcpAcceptor();

public:
    // 创建socket , bind, listen
    void init();
    // 调用accept_hook()获取新连接，注册到epoll，等待新连接唤醒协程
    int toAccept();

public:
    NetAddress::ptr getPeerAddr()
    {
        return m_peer_addr;
    }

    NetAddress::ptr getLocalAddr()
    {
        return m_local_addr;
    }

private:
    int m_family {-1};
    int m_fd {-1};

    NetAddress::ptr m_local_addr {nullptr};
    NetAddress::ptr m_peer_addr {nullptr};
};

// 处理TCP服务，分为主协程：不断执行Reactor的loop函数运行epoll_wait直到listenfd可读
// 另一个协程是不断循环accept 返回一个连接就交给一个IO线程处理(subReactor)
class TcpServer{
public:
    typedef std::shared_ptr<TcpServer> ptr;

public:
    TcpServer(NetAddress::ptr addr, ProtocalType type = TinyPb_Protocal);

    ~TcpServer();

public:
    void start();
    // 连接之后需要一个协程执行io操作
    void addCoroutine(tinyrpc::Coroutine::ptr cor);

    bool registerService(std::shared_ptr<google::protobuf::Service> service);

    // bool registerHttpServlet(const std::string& url_path, HttpServlet::ptr servlet);

private:
    void mainAcceptCorFunc();

    void clearClientTimerFunc();

private:
    NetAddress::ptr m_addr;     // 连接地址

    TcpAcceptor::ptr m_acceptor;// 接受处理

    int m_tcp_counts {0};     // 连接个数

    Reactor* m_main_reactor {nullptr}; // 一个主reactor负责监听

    bool m_is_stop_accept {false};   // 暂停连接标志

    Coroutine::ptr m_accept_cor;  // 设置accept的协程，用在accept_hook

    IOThreadPool::ptr m_io_pool;  // 线程池，一个线程对应一个连接，一个线程对应多个协程

    TimerEvent::ptr m_clear_clent_timer_event {nullptr}; // 一个客户端不能长时间占用连接，定时处理

};


    
} // namespace tinyrpc



#endif