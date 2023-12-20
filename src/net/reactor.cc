#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <assert.h>
#include <cstring>
#include <algorithm>

#include "src/comm/log.h"
#include "src/coroutine/coroutine.h"
#include "reactor.h"
#include "mutex.h"
#include "fd_event.h"
#include "timer.h"
#include "src/coroutine/coroutine.h"
#include "src/coroutine/coroutine_hook.h"


extern read_fun_ptr_t g_sys_read_fun; // 系统read
extern write_fun_ptr_t g_sys_write_fun; // 系统write hook.h

namespace tinyrpc{

static thread_local Reactor* t_reactor_ptr = nullptr;
static thread_local int t_max_epoll_timeout = 10000; //ms

static CoroutineTaskQueue* t_couroutine_task_queue = nullptr;

Reactor::Reactor()
{
    // 一个线程最多创建一个reactor对象。
    if(t_reactor_ptr != nullptr)
    {
        ErrorLog << "this thread has already create a reactor";
        Exit(0);  // log.cc的退出，将异步日志线程执行join开始写入日志
    }

    m_tid = gettid(); // 系统调用拿取当前线程id 

    DebugLog << "thread[" << m_tid << "] succ create a reactor object";
    t_reactor_ptr = this;  // 当前reactor分给当前线程


    // 创建epoll，返回描述符。参数现在不起作用，大于1就行，表示能放几个fd，系统决定。
    if((m_epfd = epoll_create(1)) <= 0)
    {
        ErrorLog << "start server error. epoll_create error, sys error=" << strerror(errno);
		Exit(0);
    }
    else
        DebugLog << "m_epfd = " << m_epfd;
    /**
     * 
     * 创建一个eventfd对象，或者说打开一个eventfd的文件，类似普通文件的open操作。

该对象是一个内核维护的无符号的64位整型计数器。初始化为initval的值。

flags可以以下三个标志位的OR结果：

EFD_CLOEXEC：FD_CLOEXEC，简单说就是fork子进程时不继承，对于多线程的程序设上这个值不会有错的。
EFD_NONBLOCK：文件会被设置成O_NONBLOCK，一般要设置。
EFD_SEMAPHORE：（2.6.30以后支持）支持semophore语义的read，简单说就值递减1。
这个新建的fd的操作很简单：

read(): 读操作就是将counter值置0，如果是semophore就减1。

write(): 设置counter的值。

注意，还支持epoll/poll/select操作，当然，以及每种fd都都必须实现的close。
    */
   // 创建一个唤醒事件的fd
    if((m_wake_fd = eventfd(0, EFD_NONBLOCK)) <= 0)
    {
        ErrorLog << "start server error. event_fd error, sys error=" << strerror(errno);
		Exit(0);
    }

    DebugLog << "wakefd = " << m_wake_fd;
    addWakeupFd();
}

Reactor::~Reactor()
{
   // 1. 关闭epoll fd
   // 2. 删除timer
   // 3. t_reactor_ptr 置空

   DebugLog << "~Reactor";
   close(m_epfd);
   if(m_timer != nullptr)
   {
        delete m_timer;
        m_timer = nullptr;
   } 
   t_reactor_ptr = nullptr;
}

Reactor* Reactor::getReactor()
{
    if(t_reactor_ptr == nullptr)
    {
        DebugLog << "Create new Reactor";
        t_reactor_ptr = new Reactor();
    }

    return t_reactor_ptr;
}

bool Reactor::isLoopThread() const
{
    // 当前reactor是当前线程创建的
    if(m_tid == gettid())
        return true;
    else
        return false;
}

// 添加event到当前的线程的reactor中
// 要上锁，仅仅只是这个reactor的创建线程能操作
void Reactor::addEventInLoopThread(int fd, epoll_event event)
{
    // 1. 判断fds中有没有，有就设置操作为MOD
    // 2. 设置event
    // 3. 如果是新增的就加入fds中

    assert(isLoopThread());
    
    int op = EPOLL_CTL_ADD;
    bool is_add = true;
    // 查找是否已经存在，存在就是MOD
    auto it = find(m_fds.begin(), m_fds.end(), fd);
    if(it != m_fds.end())
    {
        is_add = false;
        op = EPOLL_CTL_MOD;
    }

    // 设置
    if(epoll_ctl(m_epfd, op, fd, &event) != 0)
    {
        ErrorLog << "epoo_ctl error, fd[" << fd << "], sys errinfo = " << strerror(errno);
		return;
    }

    // 新增的加入m_fds
    if(is_add)
        m_fds.emplace_back(fd);
    
    DebugLog << "epoll_ctl add succ, fd[" << fd << "]"; 
}
// 删除
// 1. 在epoll中删除
// 2. 在记录fd的m_fds中删除，每个线程都有一个m_fds
void Reactor::delEventInLoopThread(int fd)
{
    assert(isLoopThread());

    auto it = find(m_fds.begin(), m_fds.end(), fd);
    if(it != m_fds.end()) // 这个删除的fd不是这个线程的
    {
        DebugLog << "fd[" << fd << "] not in this loop";
		return;
    }

    int op = EPOLL_CTL_DEL;

    // epoll 删除
    if((epoll_ctl(m_epfd, op, fd, nullptr)) != 0)
    {
        ErrorLog << "epoo_ctl error, fd[" << fd << "], sys errinfo = " << strerror(errno);
    }
    // m_fds删除
    m_fds.erase(it);
    DebugLog << "del succ, fd[" << fd << "]";
}

// 设置wakeup fd，作用是唤醒reactor
void Reactor::wakeup()
{
    // 没有在循环 
    if(!m_is_looping)
        return;
    
    uint64_t tmp = 1;
    uint64_t* p = &tmp;
    // 使用write就能写入fd
    if(g_sys_write_fun(m_wake_fd, p, 8) != 8)
    {
        ErrorLog << "write wakeupfd[" << m_wake_fd << "] error";
    }

}

// 添加wakeupfd到epoll中
void Reactor::addWakeupFd()
{
    int op = EPOLL_CTL_ADD;

    epoll_event event;
    event.data.fd = m_wake_fd;
    event.events = EPOLLIN;

    if ((epoll_ctl(m_epfd, op, m_wake_fd, &event)) != 0) 
    {
		ErrorLog << "epoo_ctl error, fd[" << m_wake_fd << "], errno=" << errno << ", err=" << strerror(errno) ;
	}

    m_fds.emplace_back(m_wake_fd);
}

// 加入event到当前循环线程中，需要上锁
void Reactor::addEvent(int fd, epoll_event event, bool is_wakeup /*true*/)
{
    if(fd == -1)
    {
        ErrorLog << "add error. fd invalid, fd = -1";
        return;
    }

    // 判断当前reactor是否是当前的循环线程创建的，是就加入
    if(isLoopThread())
    {
        addEventInLoopThread(fd, event);
        return;
    }

    // 当前reactor不是当前循环线程创建的，加入待定队列中，但是要上锁
    {
        Mutex::Lock lock(m_mutex);
        // 加入到当前reactor的待定队列中，等待切换到对应线程再加入epoll
        m_pending_add_fds.insert(std::pair<int, epoll_event>(fd, event));
    }

    if(is_wakeup)
        wakeup();
}

// 删除，同样reactor需要对应
void Reactor::delEvent(int fd, bool is_wakeup /*true*/)
{
    if(fd == -1)
    {
        ErrorLog << "add error. fd invalid, fd = -1";
        return;
    }

    if(isLoopThread())
    {
        delEventInLoopThread(fd);
        return;
    }

    {// 为什么不解锁？因为出了定义域就触发析构函数，自动解锁
        Mutex::Lock lock(m_mutex);
        m_pending_del_fds.emplace_back(fd);
    }

    if(is_wakeup)
    {
        wakeup();
    }
}

/*
    ----------------------------------reactor主循环-------------------------------
*/
void Reactor::loop()
{
    assert(isLoopThread());

    if(m_is_looping) // 已经在循环中了
        return;
    
    m_is_looping = true;
    m_stop_flag = false;

    Coroutine* first_coroutine = nullptr; // epoll_wait的第一个协程

    while(!m_stop_flag)
    {
        const int MAX_EVENTS = 10;
        epoll_event re_events[MAX_EVENTS + 1];

        // 第一个协程有效唤醒
        if(first_coroutine)
        {
            tinyrpc::Coroutine::Resume(first_coroutine);
            first_coroutine = NULL;
        }

        // 主协程不需要被唤醒，io协程才需要，全部唤醒，就是专门为子reacotr设置的，唤醒所有读写事件的协程进行处理。
        if(m_reactor_type != MainReactor)
        {
            FdEvent* ptr = NULL;
            while(1)  // 不断唤醒fdEvent的协程，直到全部唤醒返回nullptr跳出循环。
            {
                ptr = CoroutineTaskQueue::getCoroutineTaskQueue()->pop(); // 拿一个任务队列中的协程队列中的Fdevent，有就直接拿，没有new
                if(ptr)
                {
                    ptr->setReactor(this);
                    tinyrpc::Coroutine::Resume(ptr->getCoroutine());
                }
                else
                    break;
            }
        }


        // 执行所有待定队列中的任务
        Mutex::Lock lock(m_mutex);

        std::vector<std::function<void()>> tmp_tasks;
        tmp_tasks.swap(m_pending_tasks);
        lock.unlock();
            // 执行回调函数
        for(auto task : tmp_tasks)
        {
            if(task)
                task();
        }


        // 进入epoll_wait
        int rt = epoll_wait(m_epfd, re_events, MAX_EVENTS, t_max_epoll_timeout);

        if(rt < 0)
        {
            ErrorLog << "epoll_wait error, skip, errno=" << strerror(errno);
        }
        else
        {
            // 1. 反复检查返回的事件个数，放在re_events
            for(int i = 0; i < rt; ++i)
            {
                epoll_event one_event = re_events[i];
                // 1. 1读事件, 唤醒事件，这是在唤醒reactor
                if(one_event.data.fd == m_wake_fd && (one_event.events & READ))
                {
                    char buf[8];
                    while(1)  // 使用系统read，读出一个1，是wakeup函数写入的，目的就是为了唤醒epoll_wait的reactor。如果需要马上处理事件，就直接唤醒
                    {
                        if((g_sys_read_fun(m_wake_fd, buf, 8)) == -1 && errno == EAGAIN)
                            break;
                        // 下一步就开始处理事件了,所以wakeup函数只是需要写入一个数据到wake_fd就能唤醒epoll
                    }
                }
                else
                {
                    // 加入到event.data中的是FDEvent指针。
                    tinyrpc::FdEvent* ptr = (tinyrpc::FdEvent*)one_event.data.ptr;
                    if(ptr != nullptr)
                    {
                        int fd = ptr->getFd();

                        // 不是读写事件出现了错误
                        // 1. 判断是否是读写以外的事件，如果是就添加报错日志，删除这个无效fd，但是不影响正常运行
                        if((!(one_event.events & EPOLLIN)) && (!(one_event.events & EPOLLOUT)))
                        {
                            ErrorLog << "socket [" << fd << "] occur other unknow event:[" << one_event.events << "], need unregister this socket";
                            delEventInLoopThread(fd); // 删除这个无效的fd
                        }
                        else
                        {
                            // 如果fd注册了协程，说明是之前的事件，现在唤醒了
                            if(ptr->getCoroutine())
                            {
                                // 这是epoll_wait返回的第一个fd的协程，直接设置跳过到最前面唤醒协程，因为所有的协程任务队列都应该加mutex?
                                if(!first_coroutine)
                                {
                                    first_coroutine = ptr->getCoroutine();
                                    continue;
                                }
                                // 子协程，负责io,不加入循环中，而是加入到协程任务队列中。这个reactor loop是主协程用于处理连接的
                                // 加入到协程任务队列中，在最前面会不断唤醒所有协程，在协程中执行子reactor进行io
                                if(m_reactor_type == SubReactor)
                                {
                                    delEventInLoopThread(fd);
                                    ptr->setReactor(NULL);
                                    CoroutineTaskQueue::getCoroutineTaskQueue()->push(ptr);
                                }
                                else
                                {
                                    // 主reactor
                                    tinyrpc::Coroutine::Resume(ptr->getCoroutine());
                                    if(first_coroutine)
                                        first_coroutine = NULL;
                                }
                            }
                            else   // 如果没有注册协程，是新fd，设置回调函数，注册epoll事件       协程设置是在Enentfd中的    
                            {
                                std::function<void()> read_cb;
                                std::function<void()> write_cb;
                                read_cb = ptr->getCallBack(READ);
                                write_cb = ptr->getCallBack(WRITE);

                                // 如果是定时器事件，就执行，不用写入，执行定时器会唤醒其他任务
                                // 应该是执行onTimer()
                                if(fd == m_timer_fd)
                                {
                                    read_cb();
                                    continue;
                                }

                                // 判断读写事件
                                if(one_event.events & EPOLLIN)
                                {
                                    Mutex::Lock lock(m_mutex); // 出了定义域自动解锁
                                    m_pending_tasks.emplace_back(read_cb);
                                }
                                if(one_event.events & EPOLLOUT)
                                {
                                    Mutex::Lock lock(m_mutex);
                                    m_pending_tasks.emplace_back(write_cb);
                                }
                            }
                        }
                    }
                }
            }// end for

            // 添加或者删除不是本线程（是第一次加入本线程，但是没有注册到recator的事件）的待定事件
            std::map<int, epoll_event> tmp_add;
            std::vector<int> tmp_del;

            {
                Mutex::Lock lock(m_mutex);
                tmp_add.swap(m_pending_add_fds);
                m_pending_add_fds.clear();  // 其实交换之后就是空的

                tmp_del.swap(m_pending_del_fds);
                m_pending_del_fds.clear();
            }
            // 也就是add里面是本线程缺少的，现在加入
            //del是多出来的，删除
            // 执行操作
            for(auto i = tmp_add.begin(); i != tmp_add.end(); ++i)
            {
                addEventInLoopThread((*i).first, (*i).second);
            }
            for(auto i = tmp_del.begin(); i != tmp_del.end(); ++i)
            {
                delEventInLoopThread((*i));
            }

        }// end rt else

    } // end while(!m_stop_flag)

    DebugLog << "reactor loop end";
    m_is_looping = false;
}

// 暂停
void Reactor::stop()
{
    if(!m_stop_flag && m_is_looping)
    {
        m_stop_flag = true;
        wakeup();
    }
}
// 添加任务在reactor中
void Reactor::addTask(std::function<void()> task, bool is_wakeup /*=true*/) 
{
    {
        Mutex::Lock lock(m_mutex);
        m_pending_tasks.emplace_back(task);
    }
    if(is_wakeup)
        wakeup();  // 唤醒reactor进行处理事件
}
// 添加多个任务
void Reactor::addTask(std::vector<std::function<void()>> task, bool is_wakeup /* =true*/) 
{
    if(task.size() == 0)
        return;
    
    {
        Mutex::Lock lock(m_mutex);
        m_pending_tasks.insert(m_pending_tasks.end(), task.begin(), task.end());
    }
    if(is_wakeup)
        wakeup();
}

// 为reactor添加协程，也就是唤醒几个协程用
void Reactor::addCoroutine(tinyrpc::Coroutine::ptr cor, bool is_wakeup /*=true*/)
{
    auto func = [cor](){
        tinyrpc::Coroutine::Resume(cor.get()); // 唤醒cor的原始指针，这个匿名函数会在loop中的task()中执行，唤醒协程
    };
    addTask(func, is_wakeup);
}

Timer* Reactor::getTimer()
{
    if(!m_timer)
    {
        m_timer = new Timer(this);
        m_timer_fd = m_timer->getFd(); // 定时器fd
    }
    return m_timer;
}

pid_t Reactor::getTid() 
{
  return m_tid;
}

void Reactor::setReactorType(ReactorType type)
{
    m_reactor_type = type;
}

/*
------------------------CoroutineTaskQueue----------------------
*/

CoroutineTaskQueue* CoroutineTaskQueue::getCoroutineTaskQueue() 
{
    if(t_couroutine_task_queue)
        return t_couroutine_task_queue;
    
    t_couroutine_task_queue = new CoroutineTaskQueue();
    return t_couroutine_task_queue;
}

// fd事件放入协程队列
void CoroutineTaskQueue::push(FdEvent* cor)
{
    Mutex::Lock lock(m_mutex);
    m_task.push(cor);
    lock.unlock();
}

// 拿出一个fd事件
FdEvent* CoroutineTaskQueue::pop()
{
    FdEvent* re = nullptr;

    Mutex::Lock lock(m_mutex);
    
    if(m_task.size() >= 1)
    {
        re = m_task.front();
        m_task.pop();
    }

    lock.unlock();
}
    
} // namespace tinyrpc
