#ifndef SRC_NET_REACTOR_H
#define SRC_NET_REACTOR_H

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <vector>
#include <atomic>
#include <map>
#include <functional>
#include <queue>


#include "src/coroutine/coroutine.h"
#include "mutex.h"

/*

分为主从reactor
    主reactor ： 负责accept的fd使用epoll，之后注册到从reactor
    从reactor : 负责read write的fd, 使用协程调用，整个过程是同步的写法，异步的调用
                通过read唤醒协程，注册可读。之后挂起协程继续监听，等待可读事件再唤醒协程进行read

*/

namespace tinyrpc{

enum ReactorType{
    MainReactor = 1,    // 主reactor, 仅仅是主线程设置这个类型
    SubReactor = 2      // 从， 所有io线程（协程）都是这个类型
};

class FdEvent;
class Timer;

/*

---------------------Reactor 类

*/
class Reactor{
public:
    typedef std::shared_ptr<Reactor> ptr;

public:
    explicit Reactor();
    ~Reactor();

public:
    // 添加事件
    void addEvent(int fd, epoll_event event, bool is_wakeup = true);
    // 删除事件
    void delEvent(int fd, bool is_wakeup = true);
    // 添加任务(回调函数)
    void addTask(std::function<void()> task, bool is_wakeup = true);
    // 添加多个任务
    void addTask(std::vector<std::function<void()>> tasks, bool is_wakeup = true);
    // 添加协程
    void addCoroutine(tinyrpc::Coroutine::ptr cor, bool is_wakeup = true);
    // 唤醒协程
    void wakeup();
    // 事件循环
    void loop();
    // 暂停
    void stop();
    // 获取事件定时器
    Timer* getTimer();
    // 获取线程id
    pid_t getTid();
    // 设置本reactor的主从类型
    void setReactorType(ReactorType type);

public:
    static Reactor* getReactor();

private:
    void addWakeupFd();
    bool isLoopThread() const;
    void addEventInLoopThread(int fd, epoll_event event);
    void delEventInLoopThread(int fd);

private:
    int m_epfd {-1}; // 
    int m_wake_fd {-1};   // 本次事件到达唤醒的fd
    int m_timer_fd {-1};  // 计时器到时的fd
    bool m_stop_flag {false};
    bool m_is_looping {false};
    bool m_is_init_timer {false};
    pid_t m_tid {0};     // 线程id

    Mutex m_mutex;

    std::vector<int> m_fds;     // 已经
    std::atomic<int> m_fd_size;  // fd个数

    // epoll_event结构体，包含epoll事件和epoll数据
    std::map<int, epoll_event> m_pending_add_fds; // 加入fd到epoll loop
    std::vector<int> m_pending_del_fds;            // 删除fd ，从epoll loop ，记录的是加入的编号

    Timer* m_timer {nullptr}; // 事件定时器

    ReactorType m_reactor_type {SubReactor};
};


/*

---------------------CoroutineTaskQueue 类

*/

class CoroutineTaskQueue
{
public:
    static CoroutineTaskQueue* getCoroutineTaskQueue();

    void push(FdEvent* fd);

    FdEvent* pop();

private:
    std::queue<FdEvent*> m_task;
    Mutex* m_mutex;
};



} // namespace tinyrpc



#endif 