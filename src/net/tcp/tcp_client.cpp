#include <sys/socket.h>
#include <arpa/inet.h>
#include "src/comm/log.h"
#include "src/coroutine/coroutine.h"
#include "src/coroutine/coroutine_hook.h"
#include "src/coroutine/coroutine_pool.h"
#include "src/net/net_address.h"
#include "src/net/tcp/tcp_client.h" 
#include "src/comm/error_code.h"
#include "src/net/timer.h"
#include "src/net/fd_event.h"
// #include "src/net/http/http_codec.h"
#include "src/net/tinypb/tinypb_codec.h"
#include "tcp_client.h"



namespace tinyrpc {

// 初始化socket
TcpClient::TcpClient(NetAddress::ptr addr, ProtocalType type)
: m_peer_addr(addr)
{
    // 协议族
    m_family = m_peer_addr->getFamily();
    // 创建socket
    m_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(m_fd == -1)
    {
        ErrorLog << "call socket error, fd=-1, sys error=" << strerror(errno);
    }
    DebugLog << "TcpClient() create fd = " << m_fd;

    // 设置地址，准备connect ,本地地址，端口为0
    m_local_addr = std::make_shared<tinyrpc::IPAddress>("127.0.0.1", 0);
    // 设置子reactor进行io
    m_reactor = Reactor::getReactor();

    // 设置编解码类型
    if(type == Http_Protocal)
    {
        // http编解码
    }
    else
    {
       // m_codec = std::shared_ptr<TinyPbCodeC>();  // 挑出一个bug，make_shared写成了shared_ptr
       m_codec = std::make_shared<TinyPbCodeC>();
    }


}

TcpClient::~TcpClient()
{
    if(m_fd > 0)
    {
        FdEventContainer::getFdContainer()->getFdEvent(m_fd)->unregisterFromReactor(); // 取消fd监听，也就是设置DEL
        close(m_fd);
        DebugLog << "~TcpClient() close fd = " << m_fd;
    }
}

// 重新创建socket,使用新的fd
void TcpClient::resetFd()
{
    // 先取消注册
    FdEventContainer::getFdContainer()->getFdEvent(m_fd)->unregisterFromReactor();
    close(m_fd);
    // 重新创建
    m_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(m_fd == -1)
    {
        ErrorLog << "call socket error, fd=-1, sys error=" << strerror(errno);
    }
}

// 使用协程进行收发,发送的是远程调用，接受的是回复的结果包
int TcpClient::sendAndRecvTinyPb(const std::string &msg_no, TinyPbStruct::pb_ptr &res)
{
    bool is_timeout = false;
    // 拿出当前客户端使用的协程，设置回调函数，加入到超时事件中，如果时间到了触发读写事件，执行回调函数唤醒协程
    tinyrpc::Coroutine* cur_cor = tinyrpc::Coroutine::getCurrentCoroutine();

    auto timer_cb = [this, &is_timeout, cur_cor]()
    {
        InfoLog << "TcpClient timer out event occur";
        is_timeout = true;
        this->m_connection->setOverTimeFlag(true);
        tinyrpc::Coroutine::Resume(cur_cor);
    };

    TimerEvent::ptr event = std::make_shared<TimerEvent>(m_max_timeout, false, timer_cb);
    m_reactor->getTimer()->addTimerEvent(event);
    DebugLog << "add rpc timer event, timeout on " << event->m_arrive_time;

    // 执行，直到超时或者连接出错
    while(!is_timeout)
    {
        DebugLog << "begin to connect";
        // 进行connect，使用前判断状态，并且使用hook
        if(m_connection->getState() != Connected)
        {
            int rt = connect_hook(m_fd, reinterpret_cast<sockaddr*>(m_peer_addr->getSockAddr()), m_peer_addr->getSockLen());
            if(rt == 0) // 没有阻塞，直接连接成功了
            {
                DebugLog << "connect [" << m_peer_addr->toString() << "] succ!";
                m_connection->setUpClient(); // 设置已连接状态
                break;
            }

            // 下面是阻塞状态，没有连接成功
            resetFd(); // 失败关闭，重新创建
            if(is_timeout) // 连接超时，goto跳转
            {
                InfoLog << "connect timeout, break";
                goto err_deal;
            }
            if(errno == ECONNREFUSED) // 拒绝连接，可能地址啥的错了
            {
                std::stringstream ss;
                ss << "connect error, peer[ " << m_peer_addr->toString() << "] closed";
                m_err_info = ss.str();
                ErrorLog << "cancle overtime event, err info=" << m_err_info;
                // 出错了，删除之前的event
                m_reactor->getTimer()->delTimerEvent(event);
                return ERROR_PEER_CLOSED;
            }
            if(errno == EAFNOSUPPORT) // 使用了和定义的协议不相同的地址，比如ipv4用到了ipv6地址
            {
                std::stringstream ss;
                ss << "connect cur sys ror, errinfo is " << std::string(strerror(errno)) <<  " ] closed.";
                m_err_info = ss.str();
                ErrorLog << "cancle overtime event, err info=" << m_err_info;
                m_reactor->getTimer()->delTimerEvent(event);
                return ERROR_CONNECT_SYS_ERR;
            }
        }
        else // 已经连接，跳出
        {
            break;
        }
    }

    // 再次判断是否已连接，不是就删除事件
    if(m_connection->getState() != Connected)
    {
        std::stringstream ss;
        ss << "connect peer addr[" << m_peer_addr->toString() << "] error. sys error=" << strerror(errno);
        m_err_info = ss.str();
        m_reactor->getTimer()->delTimerEvent(event);
        return ERROR_FAILED_CONNECT;
    }

    // 已连接，进行设置
        // 已连接状态设置
    m_connection->setUpClient();
    m_connection->output();    ///  !!!!!! 这是客户端，是先进行发送的，发送远程调用，是在chaannel->CallMethod中，先对请求信息编码到m_writer_buffer中，这里对这个buffer进行发送远程调用
        // 发送超时
    if (m_connection->getOverTimerFlag()) 
    {
        InfoLog << "send data over time";
        is_timeout = true;
        goto err_deal;
    }

    while(!m_connection->getResPackageData(msg_no, res))  // 没有获取到服务端返回的调用内容，进行读取远程调用编码给服务器,回复包信息是在execture这步保存的，上面调用了就能得到返回的数据
    {
        DebugLog << "redo getResPackageData";
        m_connection->input();  // 如果服务端没有执行，input只有服务端才使用，就调用input把刚才的output发送的进行读取，算是手动调用服务端进行处理

        if(m_connection->getOverTimerFlag())
        {
            InfoLog << "read data over time";
            is_timeout = true;
            goto err_deal;
        }

        if(m_connection->getState() == Closed)
        {
            InfoLog << "peer close";
            goto err_deal;
        }

        m_connection->execute(); // 读取了要进行解码，编码
    }

    m_reactor->getTimer()->delTimerEvent(event); // 执行完了删除
    m_err_info = "";
    return 0;


// 处理连接失败的问题
err_deal:
    // 连接失败，重新创建socket等待下次连接。
    FdEventContainer::getFdContainer()->getFdEvent(m_fd)->unregisterFromReactor();
    close(m_fd);

    m_fd = socket(AF_INET, SOCK_STREAM, 0);
    std::stringstream ss;
    if(is_timeout) // 是因为超时链接失败的
    {
        ss << "call rpc falied , over " << m_max_timeout << "ms";
        m_err_info = ss.str();

        m_connection->setOverTimeFlag(false); // 复原超时标志
        return ERROR_RPC_CALL_TIMEOUT;
    }
    else  // 不是因为超时连接失败的，是地址问题
    {
        ss << "call rpc falied, peer closed [" << m_peer_addr->toString() << "]";
        m_err_info = ss.str();
        return ERROR_PEER_CLOSED;
    }
    
}

void TcpClient::stop()
{
    if(!m_is_stop)
    {
        m_is_stop = true;
        m_reactor->stop();
    }
}
TcpConnection *TcpClient::getConnection()
{

    if (!m_connection.get()) 
    {
        m_connection = std::make_shared<TcpConnection>(this, m_reactor, m_fd, 128, m_peer_addr);
    }
    return m_connection.get();
}
} // namespace tinyrpc
