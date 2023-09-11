#ifndef SRC_NET_TIMER_H
#define SRC_NET_TIMER_H

#include <time.h>
#include <memory>
#include <map>
#include <functional>
#include "src/net/mutex.h"
#include "src/net/reactor.h"
#include "src/net/fd_event.h"
#include "src/comm/log.h"

/*

定时器做的事是：
    TimerEvent : 用于唤醒，取消定时任务
    FdEvent ： 创建定时fd，加入epoll，取消fd，时间重设等等。

    设置一个timerFd，注册到epoll。每次有定时事件加入就设置timerFd事件。一个Timerfd可以设置多个事件
    每次检查定时到期就删除这些定时事件，通过一个m_pending_events进行保存。通过epoll不断唤醒定时事件。
*/


namespace tinyrpc{
    
// signed long
int64_t getNowMs();

class TimerEvent{
public:
    // typedef std::shared_ptr<TimerEvent> ptr;
    using ptr = std::shared_ptr<TimerEvent>;

public:
    TimerEvent(int64_t interval, bool is_repeated, std::function<void()> task)
    : m_interval(interval),
      m_is_repeated(is_repeated),
      m_task(task)
    {
        // 设置一个间隔
        m_arrive_time = getNowMs() + m_interval;
        DebugLog << "timeevent will occur at " << m_arrive_time;
    }

    // 重新设置任务发生时间
    void resetTime()
    {
        m_arrive_time = getNowMs() + m_interval;
        m_is_cancled = false;
    }

    // 唤醒
    void wake()
    {
        m_is_cancled = false;
    }
    // 取消任务
    void cancle()
    {
        m_is_cancled = true;
    }
    // 取消任务重复
    void cancleRepeated()
    {
        m_is_repeated = false;
    }

public:
    int64_t m_arrive_time; // 任务执行的时间点 = 现在时间 + 任务间隔, ms
    int64_t m_interval;  // 两个任务间隔时间, ms
    bool m_is_repeated {false};  // 任务是否重复
    bool m_is_cancled {false};  // 任务是否已经取消
 
    std::function<void()> m_task;  // 任务
};

class FdEvent;

class Timer : public tinyrpc::FdEvent{  
public:
    // typedef std::shared_ptr<Timer> ptr;
    using ptr = std::shared_ptr<Timer>;

public:
    Timer(tinyrpc::Reactor* raector);

    ~Timer();

public:
    // 添加timerEvent
    void addTimerEvent(TimerEvent::ptr event, bool need_reset = true);
    // 删除
    void delTimerEvent(TimerEvent::ptr event);
    // 重设任务定时时间
    void resetArriveTime();
    // 定时器开始
    void onTimer();

private:
    // 继承了FdEvent还有以下成员
    /*
    
    int m_fd {-1};
  std::function<void()> m_read_callback;
  std::function<void()> m_write_callback;
  
  int m_listen_events {0};

  Reactor* m_reactor {nullptr};

  Coroutine* m_coroutine {nullptr};
    */
    std::multimap<int64_t, TimerEvent::ptr> m_pending_events; // 定时器事件，时间和事件pair
    RWMutex m_event_mutex;
};

} // namespace tinyrpc


#endif