#include <fcntl.h>
#include <unistd.h>
#include "fd_event.h"



namespace tinyrpc{

static FdEventContainer* g_FdContainer = nullptr;

FdEvent::FdEvent(tinyrpc::Reactor* reactor, int fd)
: m_fd(fd),
  m_reactor(reactor)
{
    if(reactor == nullptr)
    {
        ErrorLog << "create reactor first";
    }
}

FdEvent::FdEvent(int fd) 
: m_fd(fd) 
{

}

FdEvent::~FdEvent(){}

// 处理读写事件,调用回调函数
void FdEvent::handleEvent(IOEvent flag)
{
    if(flag == READ)
        m_read_callback();
    else if(flag == WRITE)
        m_write_callback();
    else
        ErrorLog << "FdEvent::handleEvent error flag";
}

// 设置回调函数
void FdEvent::setCallBack(IOEvent flag, std::function<void()> cb)
{
    if(flag == READ)
        m_read_callback = cb;
    else if(flag == WRITE)
        m_write_callback = cb;
    else
        ErrorLog << "FdEvent::handleEvent error flag";
}

// 获取回调函数
std::function<void()> FdEvent::getCallBack(IOEvent flag) const
{
    if(flag == READ)
       return m_read_callback;
    else if(flag == WRITE)
       return m_write_callback;
    
    return nullptr;
}

// 添加监事件
void FdEvent::addListenEvents(IOEvent event)
{
    // 本次事件已经在事件列表中
    if(m_listen_events & event)
    {
        DebugLog << "already has this event, skip";
        return;
    }

    m_listen_events |= event; // 或加入到监听事件
    updateToReactor();
}

// 删除监听事件
void FdEvent::delListenEvents(IOEvent event)
{
    if(m_listen_events & event)
    {
        DebugLog << "delete succ";
        m_listen_events &= ~event; // 取反与，相当于删除
        updateToReactor();
        return;
    }

    DebugLog << "this event not exist, skip";
}

// 事件添加或者删除，更新到reactor框架
void FdEvent::updateToReactor()
{
    epoll_event event;
    event.events = m_listen_events;
    event.data.ptr = this;  // epoll_data是一个联合体，epoll不关系放啥，是给用户携带信息的，epoll_wait返回的结构体也带这个data，用户自己判断使用，这里放了this标识
    // 还没有创建reactor
    if(!m_reactor)
    {
        // 使用一个单例模式唯一标识一个reactor实例
        m_reactor = tinyrpc::Reactor::getReactor();
    }

    m_reactor->addEvent(m_fd, event); //fd和相关事件加入到reactor
}

// 在reactor中取消本fd注册的所有事件，一个fd可以有很多事件
void FdEvent::unregisterFromReactor()
{
    if(!m_reactor)
        m_reactor = tinyrpc::Reactor::getReactor();
    
    m_reactor->delEvent(m_fd);
    m_listen_events = 0;
    m_read_callback = nullptr;
    m_write_callback = nullptr;
}

int FdEvent::getFd() const
{
    return m_fd;
}
void FdEvent::setFd(const int fd)
{
    m_fd = fd;
}

int FdEvent::getListenEvents() const
{
    return m_listen_events;
}
Reactor* FdEvent::getReactor() const
{
    return m_reactor;
}
void FdEvent::setReactor(Reactor* r)
{
    m_reactor = r;
}

// 设置非阻塞fd， 一般使用了epoll和ET模式。都要设置非阻塞，否则阻塞IO影响并发
void FdEvent::setNonBlock()
{

    /*
        设置非阻塞三部曲
        1. 拿出old_flag -> int old_flag = fcntl(fd, F_GETFL, 0);
        2. 设置O_NONBLOCK -> fcntl(fd, F_SETFL, old_flag | O_NONBLOCK);
        3. 返回旧的falg备用  -> return old_flag;
    */
    if(m_fd == -1)
    {
        ErrorLog << "error fd = -1";
        return;
    }

    int flag = fcntl(m_fd, F_GETFL, 0); // 拿取旧的flag
    if(flag & O_NONBLOCK)  // 已经设置非阻塞
    {
        DebugLog << "fd already set o_nonblock";
        return;
    }
    // 设置阻塞
    fcntl(m_fd, F_SETFL, flag | O_NONBLOCK);
    // 再拿出标志判断下是否设置成功
    flag = fcntl(m_fd, F_GETFL, 0);
    if(flag & O_NONBLOCK)
        DebugLog << "succ set o_nonblock";
    else
        ErrorLog << "set o_nonblock error";
}
// 判断是不是非阻塞
bool FdEvent::isNonBlock()
{
    if(m_fd == -1)
    {
        ErrorLog << "error, fd=-1";
        return false;
    }

    int flag = fcntl(m_fd, F_GETFL, 0);
    return (flag & O_NONBLOCK);
}

// 协程
void FdEvent::setCoroutine(Coroutine* cor)
{
    m_coroutine = cor;
}

void FdEvent::clearCoroutine() 
{
  m_coroutine = nullptr;
}

Coroutine* FdEvent::getCoroutine() 
{
  return m_coroutine;
}

/*

-------------FdEventContainer

*/
// 初始化size个FdEvent事件
FdEventContainer::FdEventContainer(int size) 
{
  for(int i = 0; i < size; ++i) 
  {
    m_fds.emplace_back(std::make_shared<FdEvent>(i));
  }
}

// 拿取指定fd的FdEvent
FdEvent::ptr FdEventContainer::getFdEvent(int fd)
{

    // 1. fd存在记录中
    RWMutex::ReadLock rlock(m_mutex);

    if(fd < static_cast<int>(m_fds.size()))   
    {
        tinyrpc::FdEvent::ptr re = m_fds[fd];
        rlock.unlock();
        return re;
    }

    rlock.unlock();

    // 2. 不在记录中，扩容，添加，拿出。
    RWMutex::WriteLock wlock(m_mutex);

    int n = (int)(fd * 1.5); // 增大1.5倍的fds容器，设置fd序号
    for(size_t i = m_fds.size(); i < n; ++i)
    {
        m_fds.emplace_back(std::make_shared<FdEvent>(static_cast<int>(i)));
    }
    tinyrpc::FdEvent::ptr re = m_fds[fd];
    wlock.unlock();
    return re;
}

FdEventContainer* FdEventContainer::getFdContainer() 
{
  if (g_FdContainer == nullptr) 
  {
    g_FdContainer = new FdEventContainer(1000); // 创建一个新容器，默认创建1000个FdEvent 
  }
  return g_FdContainer;
}
    
} // namespace tinyrpc
