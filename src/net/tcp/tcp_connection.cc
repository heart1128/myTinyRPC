#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include "src/net/tcp/tcp_connection.h"
#include "src/net/tcp/tcp_server.h"
#include "src/net/tcp/tcp_client.h"
#include "src/net/tinypb/tinypb_codec.h"
#include "src/net/tinypb/tinypb_data.h"
#include "src/coroutine/coroutine_hook.h"
#include "src/coroutine/coroutine_pool.h"
#include "src/net/tcp/tcp_connection_time_wheel.h"
#include "src/net/tcp/abstract_slot.h"
#include "src/net/timer.h"
#include "tcp_connection.h"


namespace tinyrpc{

// 服务端初始化tcp连接，需要创建一个新的Reactor，从线程池中拿出来的
tinyrpc::TcpConnection::TcpConnection(tinyrpc::TcpServer *tcp_svr, tinyrpc::IOThread *io_thread, int fd, int buff_size, NetAddress::ptr peer_addr)
:m_io_thread(io_thread), m_fd(fd), m_state(Connected), m_connection_type(ServerConnection), m_peer_addr(peer_addr)
{
    // 1. 线程池中拿出reactor
    m_reactor = m_io_thread->getReactor();

    m_tcp_svr = tcp_svr;

    // rpc内容, m_codec进行编解码
    m_codec = m_tcp_svr->getCodec();

    // 2.使用fd创建一个Fd事件
    m_fd_event = FdEventContainer::getFdContainer()->getFdEvent(fd);
    m_fd_event->setReactor(m_reactor);

    // buffer初始化，设置初始化大小
    initBuffer(buff_size);

    // 3.设置循环协程，这个协程的作用是进行rpc内容的传输和读取，同步写法异步调用，具体看流程图
    m_loop_cor = getCoroutinePool()->getCoroutineInstanse();

    DebugLog << "succ create tcp connection[" << m_state << "], fd=" << fd;
}

// 客户端连接，状态设置未连接，需要每次创建
TcpConnection::TcpConnection(tinyrpc::TcpClient *tcp_cli, tinyrpc::Reactor *reactor, int fd, int buff_size, NetAddress::ptr peer_addr)
:m_fd(fd), m_state(NotConnected), m_connection_type(ClientConnection), m_peer_addr(peer_addr)
{
    m_reactor = reactor;
    m_tcp_cli = tcp_cli;
    //客户端编码方式获取
    m_codec = m_tcp_cli->getCodeC();

    m_fd_event = FdEventContainer::getFdContainer()->getFdEvent(fd);
    m_fd_event->setReactor(m_reactor);

    initBuffer(buff_size);

    DebugLog << "succ create tcp connection[NotConnected]";
}

// 最后要处理的事就是归还协程到协程池
TcpConnection::~TcpConnection()
{
    // 只有服务端需要协程，进行读写协程
    // 客户端只是处理一下连接
    if(m_connection_type == ServerConnection)
        getCoroutinePool()->returnCoroutine(m_loop_cor);

    DebugLog << "~TcpConnection, fd=" << m_fd;
}

void TcpConnection::setUpClient()
{
    setState(Connected);
}

void TcpConnection::initBuffer(int size)
{
    // 初始化读写缓冲区大小
    m_write_buffer = std::make_shared<TcpBuffer>(size);
    m_read_buffer = std::make_shared<TcpBuffer>(size);
}

// 初始化整个服务
void TcpConnection::initServer()
{
    // 注册时间轮、设置connection的主要函数到loop协程的回调函数中、
    registerToTimeWheel();
    m_loop_cor->setCallBack(std::bind(&TcpConnection::MainServerLoopCorFunc, this));
}

// 协程添加到reactor中，epoll检查到读写就会启动主函数进行循环
void TcpConnection::setUpServer()
{
    m_reactor->addCoroutine(m_loop_cor);
}

// 注册到时间轮，处理长时间的无效链接
void TcpConnection::registerToTimeWheel()
{
    // 注册到时间轮就是将连接本身通过fresh插入到最后一个槽的vector中
    // 并且保存一个weak_slot监视指针,为了不增加引用计数，循环引用问题
    // 如果下次有input被触发，weak_slot.lock()升级一个shard_ptr再新加入槽中，增加一个引用计数
    // 每次时间间隔到了，pop之后会有一个shard_ptr被删除，引用计数-1，直到这个连接的所有指针被删除，引用计数清零执行析构进行读写关闭

    auto cb = [](TcpConnection::ptr conn)
    {
        conn->shutdownConnection(); // 读写关闭，发送四次挥手的第一步FIN
    };

    // 添加抽象槽到时间轮中
    TcpTimeWheel::TcpConnectionSlot::ptr tmp = std::make_shared<AbstractSlot<TcpConnection>>(shared_from_this(), cb);
    // 添加监视
    m_weak_slot = tmp;
    // 插入到最后一个槽，conn本身没有设置时间轮，通过服务端的时间轮添加，所以管理时间轮的是服务端
    m_tcp_svr->freshTcpConnection(tmp);
}

// 做三件事，读，处理rpc，发
void TcpConnection::MainServerLoopCorFunc()
{

    while(!m_stop)
    {
        input();

        execute();

        output();
    }
    InfoLog << "this connection has already end loop";
}

// 1、fdEvent自带协程，注册fd到reactor  2、等到协程唤醒进行读
void TcpConnection::input()
{
    // 时间到了
    if(m_is_over_time)
    {
        InfoLog << "over timer, skip input progress";
        return;
    }

    // 处理无效状态
    TcpConnectionState state = getState();
    if(state == Closed || state == NotConnected)
    {
        return;
    }

    // 主循环，直到读完数据
    bool read_all = false;
    bool close_flag = false;
    int count = 0;
    while(!read_all)
    {
        // 1. 读buffer不够，扩容2倍
        if(m_read_buffer->writeAble() == 0)
        {
            m_read_buffer->resizeBuffer(2 * m_read_buffer->getSize());
        }

        int read_count = m_read_buffer->writeAble();
        int write_index = m_read_buffer->writeIndex();

        DebugLog << "m_read_buffer size=" << m_read_buffer->getBufferVector().size() << "rd=" << m_read_buffer->readIndex() << "wd=" << m_read_buffer->writeIndex();
        // 2. 使用read_hook，尝试读一遍，读不完就进行协程yield()
        int rt = read_hook(m_fd, &(m_read_buffer->m_buffer[write_index]), read_count);
            // 有数据返回，将缓冲区调整一下，利用没有用的空间
        if(rt > 0)
        {
            m_read_buffer->recucleWrite(rt);
        }
        DebugLog << "m_read_buffer size=" << m_read_buffer->getBufferVector().size() << "rd=" << m_read_buffer->readIndex() << "wd=" << m_read_buffer->writeIndex();

        DebugLog << "read data back, fd=" << m_fd;
        count += rt;
        if(m_is_over_time)
        {
            InfoLog << "over timer, now break read function";
            break;
        }

            // read出错
        if(rt < 0)
        {
            DebugLog << "rt <= 0";
            ErrorLog << "read empty while occur read event, because of peer close, fd= " << m_fd << ", sys error=" << strerror(errno) << ", now to clear tcp connection";
            // this cor can destroy
            close_flag = true;
            break;
        } 
        else if(rt == read_count)  // 这次读缓冲区全部读满了，不能判断是不是后面还有数据，需要再次循环，开头会进行扩容
        {
            DebugLog << "read_count == rt";
            // this is possible read more data, should continue read
            continue;
        }
        else if(rt < read_count) // 读到数据了，缓冲区也没满，说明数据全部读完了，直接跳出
        {
            DebugLog << "read_count > rt";
            read_all = true;
            break;
        }
    }

    // read出错就要关闭连接，挂起这个客户端的协程
    if(close_flag)
    {
        clearClient();
        DebugLog << "peer close, now yield current coroutine, wait main thread clear this TcpConnection";
        Coroutine::getCurrentCoroutine()->setCanResume(false); // 设置协程不可唤醒，因为客户端都断开连接了
        Coroutine::Yield();
    }

    // 超时
    if(m_is_over_time)
        return;

    if(!read_all)
    {
        // 如果进入到循环中，只有两种状态，读取出错和读取成功，前面判断了读取出错的状态
        ErrorLog << "not read all data in socket buffer";
    }
    InfoLog << "recv [" << count << "] bytes data from [" << m_peer_addr->toString() << "], fd [" << m_fd << "]";

    if(m_connection_type == ServerConnection)
    {
        // 刷新时间轮
    }
}

// rpc内容解码
// 分为两种解码方式，也分为服务端和客户端
void TcpConnection::execute()
{
    // 1. 有内容读才进行解析
    while(m_read_buffer->readAble() > 0)
    {
        std::shared_ptr<AbstractData> data;
        // 2. 判断解码类型
        if(m_codec->getProtocalType() == TinyPb_Protocal)
            data = std::make_shared<TinyPbStruct>();
        else
            // http请求体
        
        // 3. 解码
        m_codec->decode(m_read_buffer.get(), data.get());
        if(!data->decode_succ)
        {
            ErrorLog << "it parse request error of fd " << m_fd;
            break;
        }

        // 4. 判断客户端和服务端
        if(m_connection_type == ServerConnection)
        {
            m_tcp_svr->getDispatcher()->dispatcher(data.get(), this); // 分发处理客户端请求，使用本conn发送出去
        }
        else if(m_connection_type == ClientConnection)
        {
            std::shared_ptr<TinyPbStruct> tmp = std::dynamic_pointer_cast<TinyPbStruct>(data); // 客户端连接，这部分就是input()读取到了回复，保存在map中
            if(tmp)
                m_reply_datas[tmp->msg_req] = tmp;
        }

    }
}
// 发送rpc内容
void TcpConnection::output()
{
    // 1. 判断超时
    if(m_is_over_time)
    {
        InfoLog << "over timer, skip output progress";
        return;
    }

    while(true)
    {
        // 判断状态
        TcpConnectionState state = getState();
        if(state != Connected)
            break;
        
        // 2. 写入
        if(m_write_buffer->readAble() == 0)
        {
            // buffer中没有内容可以发送的
            DebugLog << "app buffer of fd[" << m_fd << "] no data to write, to yiled this coroutine";
            break;
        }

        int total_size = m_write_buffer->readAble();
        int read_index = m_write_buffer->readIndex();
        // 3. 使用write_hook使用协程异步
        int rt = write_hook(m_fd, &(m_write_buffer->m_buffer[read_index]), total_size);
        // 写出错
        if(rt <= 0)
        {
            ErrorLog << "write empty, error=" << strerror(errno);
        }

        // 写成功，调整缓冲区
        DebugLog << "succ write " << rt << " bytes";
        m_write_buffer->recycleRead(rt);
        DebugLog << "recycle write index =" << m_write_buffer->writeIndex() << ", read_index =" << m_write_buffer->readIndex() << "readable = " << m_write_buffer->readAble();
        InfoLog << "send[" << rt << "] bytes data to [" << m_peer_addr->toString() << "], fd [" << m_fd << "]";
        // 判断是不是全部发送完了，没有发送完可读数据应该还有
        if(m_write_buffer->readAble() <= 0)
        {
            InfoLog << "send all data, now unregister write event and break";
            break;
        }

        if(m_is_over_time)
        {
            InfoLog << "over timer, now break write function";
            break;
        }
    }
}

// 在input()中，如果读出错调用关闭客户端
// 主要步骤是取消在reactor的注册，关闭fd,设置关闭状态
void TcpConnection::clearClient()
{
    // 已经是关闭状态
    if(getState() == Closed)
    {
        DebugLog << "this client has closed";
        return;
    }

    // 取消reactor注册
    m_fd_event->unregisterFromReactor();

    // 主循环函数暂停
    m_stop = true;

    //关闭fd
    close(m_fd_event->getFd());
    setState(Closed);

}

// 半关闭连接
void TcpConnection::shutdownConnection()
{
    TcpConnectionState state = getState();
    if (state == Closed || state == NotConnected) 
    {
        DebugLog << "this client has closed";
        return;
    }
    setState(HalfClosing);
    InfoLog << "shutdown conn[" << m_peer_addr->toString() << "], fd=" << m_fd;
    // call sys shutdown to send FIN
    // wait client done something, client will send FIN
    // and fd occur read event but byte count is 0
    // then will call clearClient to set CLOSED
    // IOThread::MainLoopTimerFunc will delete CLOSED connection
    // shutdown回等待数据发送读写完毕才发出FIN信号，并且关闭了也不会清除fd
    shutdown(m_fd_event->getFd(), SHUT_RDWR);
}


TcpBuffer* TcpConnection::getInBuffer() 
{
  return m_read_buffer.get();
}

TcpBuffer* TcpConnection::getOutBuffer() 
{
  return m_write_buffer.get();
}

AbstractCodeC::ptr TcpConnection::getCodec() const
{
    return m_codec;
}

bool TcpConnection::getResPackageData(const std::string &msg_req, TinyPbStruct::pb_ptr &pb_struct)
{
    auto it = m_reply_datas.find(msg_req);
    if(it != m_reply_datas.end())
    {
        DebugLog << "return a resdata";
        pb_struct = it->second;
        m_reply_datas.erase(it);
        return true;
    }
    DebugLog << msg_req << "|reply data not exist";
    return false;
}

TcpConnectionState TcpConnection::getState()
{
  TcpConnectionState state;
  RWMutex::ReadLock lock(m_mutex);
  state = m_state;
  lock.unlock();

  return state;
}

void TcpConnection::setState(const TcpConnectionState& state) 
{
  RWMutex::WriteLock lock(m_mutex);
  m_state = state;
  lock.unlock(); 
}

void TcpConnection::setOverTimeFlag(bool value) {
  m_is_over_time = value;
}

bool TcpConnection::getOverTimerFlag() {
  return m_is_over_time;
}

Coroutine::ptr TcpConnection::getCoroutine() {
  return m_loop_cor;
}



}