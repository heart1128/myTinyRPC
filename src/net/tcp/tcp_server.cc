#include <sys/socket.h>
#include <assert.h>
#include <fcntl.h>
#include <string.h>

#include "src/net/tcp/tcp_server.h"
#include "src/net/tcp/tcp_connection.h"
#include "src/coroutine/coroutine.h"
#include "src/coroutine/coroutine_hook.h"
#include "src/coroutine/coroutine_pool.h"
#include "src/net/tcp/io_thread.h"
#include "src/net/tinypb/tinypb_rpc_dispatcher.h"
#include "src/net/tinypb/tinypb_codec.h"

#include "src/comm/config.h"
#include "tcp_server.h"


namespace tinyrpc {

extern tinyrpc::Config::ptr gRpcConfig;


TcpAcceptor::TcpAcceptor(NetAddress::ptr net_addr)
: m_local_addr(net_addr)
{
    m_family = m_local_addr->getFamily();
}

// socket , bind, listen
void TcpAcceptor::init()
{
    // 1. 创建socket fd
    m_fd = socket(m_local_addr->getFamily(), SOCK_STREAM, 0);
    if(m_fd < 0)
    {
        ErrorLog << "start server error. socket error, sys error=" << strerror(errno);
        Exit(0);
    }

    DebugLog << "create listenfd succ, listenfd=" << m_fd;

    // 2. 设置地址重用，四次挥手的TIME_WAIT阶段
    int val = 1;
        // SOL_SOCKET设置等级，套接字得用这个
    if(setsockopt(m_fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)) < 0)
    {
        ErrorLog << "set REUSEADDR error!";
    }

    // 3. bind 正确返回0  传入地址和地址长度
    socklen_t len = m_local_addr->getSockLen();
    int rt = bind(m_fd, m_local_addr->getSockAddr(), len);
    if (rt != 0) 
    {
        ErrorLog << "start server error. bind error, errno=" << errno << ", error=" << strerror(errno);
        Exit(0); // 失败，日志线程join
    }

    DebugLog << "set REUSEADDR succ";

    // 4. listen 设置监听数，但只是建议的数量，实际会多点
    rt = listen(m_fd, 10);
    if (rt != 0) 
    {
        ErrorLog << "start server error. listen error, fd= " << m_fd << ", errno=" << errno << ", error=" << strerror(errno);
        Exit(0);
    }
}

TcpAcceptor::~TcpAcceptor()
{
    // 取消注册
    // 关闭socket fd
    FdEvent::ptr fd_event = FdEventContainer::getFdContainer()->getFdEvent(m_fd);
    fd_event->unregisterFromReactor();
    if(m_fd != -1)
        close(m_fd);
}

// 调用accept_hook, hook里面添加协程给fd，设置fd到epoll，挂起协程等待accept的READ事件
// 其实只要有用户不断连接accept就不会阻塞等待，设置hook非阻塞是为了遵循阻塞只在epoll_wait原则。
// 只要reactor写正确了一直都会有连接。listenfd是这样的，clientfd不一定，因为客户还有read/write操作。
int TcpAcceptor::toAccept()
{
    socklen_t len = 0;
    int rt = 0;

    if(m_family == AF_INET)   // IPV4
    {
        sockaddr_in cli_addr; // 设置客户端地址，送入accept会写入信息，自动分配的端口等等
        memset(&cli_addr, 0, sizeof(cli_addr));
        len = sizeof(cli_addr);

        // 调用hook
        rt = accept_hook(m_fd, reinterpret_cast<sockaddr*>(&cli_addr), &len);
        if(rt == -1)
        {
            DebugLog << "error, no new client coming, errno=" << errno << "error=" << strerror(errno);
            return -1;
        }

        InfoLog << "New client accepted succ! port:[" << cli_addr.sin_port;
        m_peer_addr = std::make_shared<IPAddress>(cli_addr); // 引用+1
    }
    else if(m_family == AF_UNIX) // unix
    {
        sockaddr_un cli_addr;
        len = sizeof(cli_addr);
        memset(&cli_addr, 0, sizeof(cli_addr));
        // call hook accept
        rt = accept_hook(m_fd, reinterpret_cast<sockaddr *>(&cli_addr), &len);
        if (rt == -1) {
            DebugLog << "error, no new client coming, errno=" << errno << "error=" << strerror(errno);
            return -1;
        }
        m_peer_addr = std::make_shared<UnixDomainAddress>(cli_addr);
    }
    else
    {
        ErrorLog << "unknown type protocol!";
        close(rt);
        return -1;
    }

    InfoLog << "New client accepted succ! fd:[" << rt <<  ", addr:[" << m_peer_addr->toString() << "]";
    return rt;	
}

/**
 * 
 *                                          TCP Server
 * 
 * 
*/
TcpServer::TcpServer(NetAddress::ptr addr, ProtocalType type /*= TinyPb_Protocal*/)
: m_addr(addr)
{
    // 1. 创建一个线程池，处理连接
    m_io_pool = std::make_shared<IOThreadPool>(gRpcConfig->m_iothread_num);

    // RPC工具，暂时没实现
    if (type == Http_Protocal) 
    {
        // m_dispatcher = std::make_shared<HttpDispacther>();
        // m_codec = std::make_shared<HttpCodeC>();
        // m_protocal_type = Http_Protocal;
    } 
    else
    {
        m_dispatcher = std::make_shared<TinyPbRpcDispacther>();
        m_codec = std::make_shared<TinyPbCodeC>();
        m_protocal_type = TinyPb_Protocal;
    }

    // 设置一个主Reactor, 子reactor都是new的，主是单例模式
    m_main_reactor = tinyrpc::Reactor::getReactor();
    m_main_reactor->setReactorType(MainReactor);


    // 设置时间轮，在reactor上运行，设置槽数和删除间隔
    m_time_wheel = std::make_shared<TcpTimeWheel>(m_main_reactor, gRpcConfig->m_timewheel_bucket_num, gRpcConfig->m_timewheel_inteval); // 连接时间轮
    // 删除事件加入reactor中的定时器中，定时器包含多个事件，有一个定时器fd，如果时间到了会发出读信号，reactor的epoll判断到，就启动定时器的onTimer()函数，
    // 这个函数启动所有到时的事件，就触发了这个函数
    m_clear_clent_timer_event = std::make_shared<TimerEvent>(10000, true, std::bind(&TcpServer::clearClientTimerFunc, this)); // 时间轮中要定时清除连接，记录的客户端也要定时清理
    m_main_reactor->getTimer()->addTimerEvent(m_clear_clent_timer_event);  // 客户端timer清除

    InfoLog << "TcpServer setup on [" << m_addr->toString() << "]";
}

void TcpServer::start()
{
    m_acceptor.reset(new TcpAcceptor(m_addr));
    // 初始化，socket,bind,listen
    m_acceptor->init();
    // 设置任务协程
    m_accept_cor = getCoroutinePool()->getCoroutineInstanse();
    m_accept_cor->setCallBack(std::bind(&TcpServer::mainAcceptCorFunc, this));

    InfoLog << "resume accept coroutine";

    tinyrpc::Coroutine::Resume(m_accept_cor.get()); // 直接执行
    //唤醒start信号量，执行子reactor的loop。负责连接
    m_io_pool->start();
    // 执行主reactor的loop，只负责监听
    m_main_reactor->loop();
}

TcpServer::~TcpServer()
{
    // 归还accept协程
    getCoroutinePool()->returnCoroutine(m_accept_cor);
    DebugLog << "~TcpServer";
}

// TcpConnection添加到时间轮
void TcpServer::freshTcpConnection(TcpTimeWheel::TcpConnectionSlot::ptr slot)
{
    auto cb = [slot, this]() mutable
    {
        this->getTimeWheel()->fresh(slot);  // 传入函数，引用计数也会加+1
        slot.reset(); // 释放slot， 因为这里lambda是拷贝使用slot，会增加一个引用计数，需要手动清除
    };
    m_main_reactor->addTask(cb);  // 添加到reactor中，addTask()马上唤醒reactor，reactor的主循环中会执行这个回调函数。
}

// 添加客户端分为两步：1、之前连接过的客户端，取消链接，建立新链接，2、新的就直接创建连接
TcpConnection::ptr TcpServer::addClient(IOThread *io_thread, int fd)
{
    //1 .查找客户端是否存在，存在就取消之前的连接，创建新链接
    auto it = m_clients.find(fd);
    if(it != m_clients.end())
    {
        // 删除旧连接
        it->second.reset();
        // 设置新连接
        DebugLog << "fd" << fd << "have exist, reset it";
        it->second = std::make_shared<TcpConnection>(this, io_thread, fd, 128, getPeerAddr()); // 在tcpconnection构造中，使用io_thread拿出一个reactor，每个线程都管理一个
        //返回新连接
        return it->second;
    }
    else
    {
        // 2. 不存在旧的，创建新连接
        DebugLog << "fd " << fd << "did't exist, new it";
        TcpConnection::ptr conn = std::make_shared<TcpConnection>(this, io_thread, fd, 128, getPeerAddr());
        m_clients[fd] = conn;
        return conn;
    }
    
}

AbstractCodeC::ptr TcpServer::getCodec()
{   
    return m_codec;
}

TcpTimeWheel::ptr TcpServer::getTimeWheel()
{
    return m_time_wheel;
}

NetAddress::ptr TcpServer::getPeerAddr()
{
    return m_acceptor->getPeerAddr();
}

NetAddress::ptr TcpServer::getLocalAddr()
{
    return m_addr;
}

IOThreadPool::ptr TcpServer::getIOThreadPool()
{
    return m_io_pool;
}

AbstractDispatcher::ptr TcpServer::getDispatcher()
{
    return m_dispatcher;
}

// 处理accept事件，通过上面的m_accept_cor唤醒执行
void TcpServer::mainAcceptCorFunc()
{
    while (!m_is_stop_accept)
    {
        // accept_hook()
        int fd = m_acceptor->toAccept();
        if(fd == -1) // 无连接事件，hook挂起等待
        {
            ErrorLog << "accept ret -1 error, return, to yield";
            Coroutine::Yield();
            continue;
        }
    
        // 拿出线程，一个线程处理一个连接，协程协助连接，每个线程都管理一个reactor
        IOThread *io_thread = m_io_pool->getIOThread();
        TcpConnection::ptr conn = addClient(io_thread, fd);
        conn->initServer();
        DebugLog << "tcpconnection address is " << conn.get() << ", and fd is" << fd;
        io_thread->getReactor()->addCoroutine(conn->getCoroutine());  // 每个reactor设置一个协程
        m_tcp_counts++;
        DebugLog << "current tcp connection count is [" << m_tcp_counts << "]";
    }
    
}


void TcpServer::clearClientTimerFunc()
{
    // 到时清理所有已经关闭连接的客户端
    for(auto &it : m_clients)
    {
        if(it.second.use_count() > 0 && it.second->getState() == Closed)
        {
            DebugLog << "TcpConection [fd:" << it.first << "] will delete, state=" << it.second->getState();
            it.second.reset(); // 将关闭的客户端连接指针指向空间清除
        }
    }
}

void TcpServer::addCoroutine(Coroutine::ptr cor)
{
    m_main_reactor->addCoroutine(cor);
}


//******************** RPC的服务注册阶段
bool TcpServer::registerService(std::shared_ptr<google::protobuf::Service> service) {
    if (m_protocal_type == TinyPb_Protocal) {
        if (service) {
            dynamic_cast<TinyPbRpcDispacther*>(m_dispatcher.get())->registerService(service);
        } else {
            ErrorLog << "register service error, service ptr is nullptr";
            return false;
        }
    } else {
        ErrorLog << "register service error. Just TinyPB protocal server need to resgister Service";
        return false;
    }
    return true;
}


//******************** 注册http应用层服务、暂时只是实现socke连接
// bool TcpServer::registerHttpServlet(const std::string& url_path, HttpServlet::ptr servlet) {
// 	if (m_protocal_type == Http_Protocal) {
// 		if (servlet) {
// 			dynamic_cast<HttpDispacther*>(m_dispatcher.get())->registerServlet(url_path, servlet);
// 		} else {
// 			ErrorLog << "register http servlet error, servlet ptr is nullptr";
// 			return false;
// 		}
// 	} else {
// 		ErrorLog << "register http servlet error. Just Http protocal server need to resgister HttpServlet";
// 		return false;
// 	}
// 	return true;
// }

    
} // namespace tinyrpc
