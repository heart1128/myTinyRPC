#include <assert.h>
#include <dlfcn.h> // 动态库显式调用，dlopen dlsym dlclose 为了拦截系统函数，做hook
#include <fcntl.h>
#include <unistd.h>
#include <cstring>

#include "src/coroutine/coroutine_hook.h"
#include "src/coroutine/coroutine.h"
#include "src/net/fd_event.h"
#include "src/net/reactor.h"
#include "src/net/timer.h"
#include "src/comm/log.h"
#include "src/comm/config.h"

/*

协程hook就是劫持系统函数，之后先判断系统函数能不能完成当前任务，如果可以就直接返回，如果不行，注册fd到epoll等待，协程挂起，等待唤醒再使用系统函数

#define Conn(x,y) x##y   表示x连接y -> xy

#define ToChar(x) #@x      给x加单引号 'x'

#define ToString(x) #x      给 x加双引号 "x"

RTLD_NEXT表示使用遇见的第一个动态库的函数name

dlsym指定动态链接库

https://blog.csdn.net/mijichui2153/article/details/109561978
*/
#define HOOK_SYS_FUNC(name) name##_fun_ptr_t g_sys_##name##_fun = (name##_fun_ptr_t)dlsym(RTLD_NEXT, #name);

HOOK_SYS_FUNC(accept);
HOOK_SYS_FUNC(read);  // g_sys_read_fun() 相当调用 系统read
HOOK_SYS_FUNC(write);
HOOK_SYS_FUNC(connect);
HOOK_SYS_FUNC(sleep);

namespace tinyrpc{
    
extern tinyrpc::Config::ptr gRpcConfig;

// hook开关，如果要使用系统read就关闭，要hook就开启
static bool g_hook = true;

void setHook(bool value)
{
    g_hook = value;
}
// 传入fdEvent和读写事件，设置fdEvent中的读写事件和对应使用的协程
void toEpoll(tinyrpc::FdEvent::ptr fd_event, int events)
{
    tinyrpc::Coroutine* cur_cor = tinyrpc::Coroutine::getCurrentCoroutine();

    // 读事件
    if(events & tinyrpc::IOEvent::READ)
    {
        DebugLog << "fd:[" << fd_event->getFd() << "], register read event to epoll";

        fd_event->setCoroutine(cur_cor);
        fd_event->addListenEvents(tinyrpc::IOEvent::READ);
    }
    // 写事件
    if(events & tinyrpc::IOEvent::WRITE)
    {
        DebugLog << "fd:[" << fd_event->getFd() << "], register write event to epoll";

        fd_event->setCoroutine(cur_cor);
        fd_event->addListenEvents(tinyrpc::IOEvent::WRITE);
    }
}

// accpet hook
int accept_hook(int sockfd, struct sockaddr* addr, socklen_t* addrlen)
{
    DebugLog << "this is hook accept";

    if(tinyrpc::Coroutine::isMainCoroutine())
    {
        DebugLog << "hook disable, call sys accept func";  // 同read
        return g_sys_accept_fun(sockfd, addr, addrlen);
    }

    tinyrpc::FdEvent::ptr fd_event = tinyrpc::FdEventContainer::getFdContainer()->getFdEvent(sockfd);
    if(fd_event->getReactor() == nullptr)
    {
        fd_event->setReactor(tinyrpc::Reactor::getReactor());
    }

    fd_event->setNonBlock();

    // accept成功返回客户端的fd, 表示现在有就直接返回，没有再进行挂起等待
    int n = g_sys_accept_fun(sockfd, addr, addrlen);
    if(n > 0)
        return n;

    toEpoll(fd_event, tinyrpc::IOEvent::READ);

    DebugLog << "accept func to yield";
    tinyrpc::Coroutine::Yield();

    fd_event->delListenEvents(tinyrpc::IOEvent::READ);
    fd_event->clearCoroutine();

    DebugLog << "accept func yield back, now to call sys accept";
	return g_sys_accept_fun(sockfd, addr, addrlen);
}

// hook作用就是在read之前使用toEpoll注册到reactor上，然后等待epoll可读事件唤醒之后清除协程，调用系统read，这样就能在有读的时候才read，不然要等待数据read
// 使用协程直接同步写代码，就不用使用回调函数等待定时器唤醒read
ssize_t read_hook(int fd, void* buf, size_t count)
{
    // 如果不使用read_hook，系统read会等待网络fd，直到有消息可读。使用了就能使得当没有消息的时候挂起协程，有可读事件的时候唤醒进行系统read

    DebugLog << "this is hook read";
    if(tinyrpc::Coroutine::isMainCoroutine()) // 主协程就是线程，只负责连接，协程才注册了读写Fd，所以直接调用read不是网络流的数据读写
    {
        DebugLog << "hook disable, call sys read func";
        return g_sys_read_fun(fd, buf, count);
    }

    // 是子协程，使用hook自定义的read
    // 这里已经是设置了fd对应的FdEvent对象，
    // 1. 取出FdEvent
    tinyrpc::FdEvent::ptr fd_event = tinyrpc::FdEventContainer::getFdContainer()->getFdEvent(fd);
    if(fd_event->getReactor() == nullptr) // 没有设置reactor,将单例设置给它
        fd_event->setReactor(tinyrpc::Reactor::getReactor());
    
    fd_event->setNonBlock();


    /*
        读一次，如果直接读完了就不用注册读事件进行循环等待了
        如果现在没有可读p的数据，说明这个sockfd读事件还没有准备就绪，需要注册READ事件到epoll等待触发协程
    */
    ssize_t n = g_sys_read_fun(fd, buf, count);
    if(n > 0)
        return n;

    toEpoll(fd_event, tinyrpc::IOEvent::READ);

    DebugLog << "read func to yield";
    tinyrpc::Coroutine::Yield();  // 协程挂起等待本fd的读事件被epoll捕捉，唤醒该协程进行read

    fd_event->delListenEvents(tinyrpc::IOEvent::READ);  // 唤醒之后已经读完了，进行后续处理
    fd_event->clearCoroutine();

    DebugLog << "read func yield back, now to call sys read";
    return g_sys_read_fun(fd, buf, count);
}

ssize_t write_hook(int fd, const void* buf, size_t count)
{
    DebugLog << "this is hook write";
    // 1. 判断协程
    if(tinyrpc::Coroutine::isMainCoroutine())
    {
        DebugLog << "hook disable, call sys write func";
        return g_sys_write_fun(fd, buf, count);
    }

    // 2. 拿出fdEnvent，进行设置
    tinyrpc::FdEvent::ptr fd_event = tinyrpc::FdEventContainer::getFdContainer()->getFdEvent(fd);
    if(fd_event->getReactor() == nullptr)
    {
        fd_event->setReactor(tinyrpc::Reactor::getReactor());
    }
    fd_event->setNonBlock();
    
    int n = g_sys_write_fun(fd, buf, count);
    if(n > 0)
        return n;
    
    toEpoll(fd_event, tinyrpc::IOEvent::WRITE);
    
    // 3. 协程挂起等待epoll唤醒
    DebugLog << "write func to yield";
    tinyrpc::Coroutine::Yield();

    fd_event->delListenEvents(tinyrpc::IOEvent::WRITE);
    fd_event->clearCoroutine();

    DebugLog << "write func yield back, now to call sys write";
    return g_sys_write_fun(fd, buf, count);
}

/*
connect需要判断连接超时的问题，处理方法就是设置一个定时器，传入回调函数，函数内容是记录超时标志和唤醒协程进行后续处理。

进行连接之后直接挂起协程。

*/
int connect_hook(int sockfd, const struct sockaddr* addr, socklen_t addrlen)
{
    DebugLog << "this is hook connect";
    // 1. 判断协程
    if(tinyrpc::Coroutine::isMainCoroutine())
    {
        DebugLog << "hook disable, call sys connect func";
        return g_sys_connect_fun(sockfd, addr, addrlen);
    }
    // reactor
    tinyrpc::Reactor* reactor = tinyrpc::Reactor::getReactor();

    tinyrpc::FdEvent::ptr fd_event = tinyrpc::FdEventContainer::getFdContainer()->getFdEvent(sockfd);
    if(fd_event->getReactor() == nullptr)
    {
        fd_event->setReactor(reactor);
    }
    fd_event->setNonBlock();
    // currention coroutine
    tinyrpc::Coroutine* cur_cor = tinyrpc::Coroutine::getCurrentCoroutine();
    
    // 成功返回0，失败返回-1并设置errno
    int n = g_sys_connect_fun(sockfd, addr, addrlen);
    
    if(n == 0)
    {
        DebugLog << "direct connect succ, return";
        return n;  
    }
    else if(errno != EINPROGRESS)  // 除了这个错误码，其他的错误码都是失败的
    {
        DebugLog << "connect error and errno is't EINPROGRESS, errno=" << errno <<  ",error=" << strerror(errno);
        return n; 
    }
    // 上面的fd_event设置了非阻塞，非阻塞connect返回-1不一定是连接错误，可能是正在连接中，此时的错误码是EINPROGRESS，可以不用返回
    DebugLog << "errno == EINPROGRESS";
    
    // connect是写事件
    toEpoll(fd_event, tinyrpc::IOEvent::WRITE);

    bool is_timeout = false; // 判断连接是否超时

    // 超时函数句柄
    auto timeout_cb = [&is_timeout, cur_cor](){
        // 设置超时标志，唤醒协程
        is_timeout = true;
        tinyrpc::Coroutine::Resume(cur_cor);
    };

    // 设置定时器事件，如果超时了，定时器事件发生，唤醒协程，进行后续事件处理。
    tinyrpc::TimerEvent::ptr event = std::make_shared<tinyrpc::TimerEvent>(gRpcConfig->m_max_connect_timeout, false, timeout_cb);

    // 把超时定时器事件加入到reactor的timer对象中
    tinyrpc::Timer* timer = reactor->getTimer();
    timer->addTimerEvent(event);

    tinyrpc::Coroutine::Yield(); // 挂起，等待超时唤醒

    // 后续处理工作，connect的写事件也要删除，后面连接成功之后还要注册写事件，那时的写事件就是传输数据了
    fd_event->delListenEvents(tinyrpc::IOEvent::WRITE);
    fd_event->clearCoroutine();

    timer->delTimerEvent(event); // 定时器没用了

    n = g_sys_connect_fun(sockfd, addr,addrlen);
    if ((n < 0 && errno == EISCONN) || n == 0)   // eisconn表示已经指定用户连接了，成功返回0
    {
		DebugLog << "connect succ";
		return 0;
	}

    if (is_timeout)
    {
        ErrorLog << "connect error,  timeout[ " << gRpcConfig->m_max_connect_timeout << "ms]";
		errno = ETIMEDOUT;
	} 
    
    DebugLog << "connect error and errno=" << errno <<  ", error=" << strerror(errno);
	return -1;
    
}

unsigned int sleep_hook(unsigned int seconds)
{
    DebugLog << "this is hook sleep";
    if (tinyrpc::Coroutine::isMainCoroutine()) 
    {
        DebugLog << "hook disable, call sys sleep func";
        // 主协程自然sleep就行，子协程使用定时器。
        return g_sys_sleep_fun(seconds);
    }

    tinyrpc::Coroutine* cur_cor = tinyrpc::Coroutine::getCurrentCoroutine();

    // 设置超时回调函数，加入定时器事件
    bool is_timeout = false;
    auto timeout_cb = [cur_cor, &is_timeout](){
        DebugLog << "onTime, now resume sleep cor";
        is_timeout = true;
        // 唤醒处理超时后续的事
        tinyrpc::Coroutine::Resume(cur_cor);
    };

    // 设置定时器事件，子协程的定时时间是传入时间的1000倍，同时还要判断1000倍的时间是否超时
    tinyrpc::TimerEvent::ptr event = std::make_shared<tinyrpc::TimerEvent>(1000 * seconds, false, timeout_cb);
    tinyrpc::Reactor::getReactor()->getTimer()->addTimerEvent(event);

    DebugLog << "now to yield sleep";
    // 因为读写事件都可能唤醒这个协程，所以当这个协程被重新唤醒的时候，必须检查是否超时，否则应该重新挂起协程
    // 因为这里设置的就是sleep协程，所以在这个协程唤醒的时候还在睡眠时间内，就不能使用协程进行读写，要重新挂起协程。
    while(!is_timeout)
    {
        tinyrpc::Coroutine::Yield();
    }

    return 0;
}


// 以下函数设置是否开启hook，不开启就返回系统函数，开启就返回自定义的hook函数
extern "C" {

int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
    if(!tinyrpc::g_hook)
        return g_sys_accept_fun(sockfd, addr, addrlen);
    else
        return tinyrpc::accept_hook(sockfd, addr, addrlen);
}


ssize_t read(int fd, void *buf, size_t count) 
{
	if (!tinyrpc::g_hook) {
		return g_sys_read_fun(fd, buf, count);
	} else {
		return tinyrpc::read_hook(fd, buf, count);
	}
}

ssize_t write(int fd, const void *buf, size_t count) 
{
	if (!tinyrpc::g_hook) {
		return g_sys_write_fun(fd, buf, count);
	} else {
		return tinyrpc::write_hook(fd, buf, count);
	}
}

int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) 
{
	if (!tinyrpc::g_hook) {
		return g_sys_connect_fun(sockfd, addr, addrlen);
	} else {
		return tinyrpc::connect_hook(sockfd, addr, addrlen);
	}
}

unsigned int sleep(unsigned int seconds) 
{
	if (!tinyrpc::g_hook) {
		return g_sys_sleep_fun(seconds);
	} else {
		return tinyrpc::sleep_hook(seconds);
	}

}

} // end extern "C"

} // namespace tinyrpc
