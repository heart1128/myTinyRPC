#ifndef SRC_NET_TCP_TCP_CONNECTION_H
#define SRC_NET_TCP_TCP_CONNECTION_H


#include <memory>
#include <vector>
#include <queue>

#include "src/comm/log.h"
#include "src/net/fd_event.h"
#include "src/net/reactor.h"
#include "src/net/tcp/tcp_buffer.h"
#include "src/coroutine/coroutine.h"
//#include "src/net/http/http_request.h"
#include "src/net/tinypb/tinypb_codec.h"
#include "src/net/tcp/io_thread.h"
#include "src/net/tcp/tcp_connection_time_wheel.h"
#include "src/net/tcp/abstract_slot.h"
#include "src/net/net_address.h"
#include "src/net/mutex.h"

/**
 * 
 * 
 * tcp_connection的作用就是处理IO读写事件，每个accept对应一个tcp_connection。
 * 使用协程hook进行异步的顺序写代码方式，进行读写挂起唤醒事件，所有读写事件都是注册在一个subReactor上
 * 
 * 
*/


namespace tinyrpc{

class TcpServer;
class TcpClient;
class IOThread;

enum TcpConnectionState{
    NotConnected = 1,  // 没连接，没有Io事件
    Connected = 2,
    HalfClosing = 3, // 半关闭，shutdown关闭，关闭读
    Closed = 4
};

// 继承enable_shared_from_this，，需要返回this智能指针的时候
class TcpConnection : public std::enable_shared_from_this<TcpConnection> {
public:
    using ptr = std::shared_ptr<TcpConnection>;

public:
    TcpConnection(tinyrpc::TcpServer* tcp_svr, tinyrpc::IOThread* io_thread, int fd, int buff_size, NetAddress::ptr peer_addr);

	TcpConnection(tinyrpc::TcpClient* tcp_cli, tinyrpc::Reactor* reactor, int fd, int buff_size, NetAddress::ptr peer_addr);

    ~TcpConnection();

public:
    void setUpClient();

    void setUpServer();

    void initBuffer(int size);

    enum ConnectionType{
        ServerConnection = 1,
        ClientConnection = 2
    };

public:
    // 半关闭socket
    void shutdownConnection();
    // 获取当前tcp连接状态
    TcpConnectionState getState();
    // 设置
    void setState(const TcpConnectionState& state);
    // tcp不能一次性传完数据，需要缓冲区保存，IO缓冲区
    TcpBuffer* getInBuffer();
    TcpBuffer* getOutBuffer();

    //rpc相关
    AbstractCodeC::ptr getCodec() const;
    bool getResPackageData(const std::string& msg_req, TinyPbStruct::pb_ptr& pb_struct);

    // 注册tcp连接到时间轮，处理无效连接
    void registerToTimeWheel();
    
    Coroutine::ptr getCoroutine();

public:
//主要的subReactor功能
    void MainServerLoopCorFunc();

    void input();

    void execute();  // rpc用

    void output();

    void setOverTimeFlag(bool value);

    bool getOverTimerFlag();

    void initServer();

private:
    void clearClient();

private:
    TcpServer* m_tcp_svr {nullptr};
    TcpClient* m_tcp_cli {nullptr};

    IOThread* m_io_thread {nullptr};
    Reactor* m_reactor {nullptr};

    int m_fd {-1}; // 连接fd

    TcpConnectionState m_state {TcpConnectionState::Connected};
    ConnectionType m_connection_type {ServerConnection};

    NetAddress::ptr m_peer_addr;

    TcpBuffer::ptr m_read_buffer;
	TcpBuffer::ptr m_write_buffer;

    Coroutine::ptr m_loop_cor;

    AbstractCodeC::ptr m_codec;

    FdEvent::ptr m_fd_event;

    
    bool m_stop {false};

    bool m_is_over_time {false};

    std::map<std::string, std::shared_ptr<TinyPbStruct>> m_reply_datas; // 通过本地rpc事务操作获得的远程函数调用的结果，通过output发送

    std::weak_ptr<AbstractSlot<TcpConnection>> m_weak_slot; //一个tcp连接抽象槽，目的是为了实现时间轮，共享指针引用计数

    RWMutex m_mutex;


};

} // tinyrpc

#endif