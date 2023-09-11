#include <sys/timerfd.h>
#include <assert.h>
#include <time.h>
#include <string.h>
#include <vector>
#include <sys/time.h>
#include <functional>
#include <map>
#include "../comm/log.h"
#include "timer.h"
#include "mutex.h"
#include "fd_event.h"
#include "../coroutine/coroutine_hook.h"

extern read_fun_ptr_t g_sys_read_fun; // sys read func

namespace tinyrpc{

int64_t getNowMs()
{
    timeval val;
    gettimeofday(&val, nullptr);
    // tv_sec : second   tv_usec : ms
    int64_t re = val.tv_sec * 1000 + val.tv_usec / 1000;
    return re;
}



Timer::Timer(tinyrpc::Reactor* reactor)
: FdEvent(reactor)
{
    // 定时器fd
    /*
    CLOCK_REALTIME :Systemwide realtime clock. 系统范围内的实时时钟
    CLOCK_MONOTONIC:以固定的速率运行，从不进行调整和复位 ,它不受任何系统time-of-day时钟修改的影响

    设置非阻塞
    int timerfd_create(int clockid, int flags);

    创建的fd可以被read , epoll, 等等使用
    
    */
    m_fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    DebugLog << "m_timer fd = " << m_fd;
    if(m_fd == -1) // 创建失败
    {
        DebugLog << "timerfd_create error";
    }
    // std::bind绑定成员函数必须后面加对象
    m_read_callback = std::bind(&Timer::onTimer, this);
    addListenEvents(tinyrpc::IOEvent::READ);  // 都是继承FDEvent的
    // updateToReactor();
}

Timer::~Timer()
{
    unregisterFromReactor();
    close(m_fd);
}

void Timer::addTimerEvent(TimerEvent::ptr event, bool need_reset)
{
    RWMutex::WriteLock lock(m_event_mutex);
    bool is_reset = false;

    if(m_pending_events.empty())
    {
        is_reset = true;
    }
    else
    {
        auto it = m_pending_events.begin();
        // 之前的到达时间小于最小的到达时间，
        if(event->m_arrive_time < (*it).second->m_arrive_time)
            is_reset = true;
    }
    // 加入到达时间和事件
    m_pending_events.emplace(event->m_arrive_time, event);
    lock.unlock();
    

    if(is_reset && need_reset)
    {
        DebugLog << "need reset timer";
        resetArriveTime();
    }
}

void Timer::delTimerEvent(TimerEvent::ptr event)
{
    event->m_is_cancled = true; // 取消

    RWMutex::WriteLock lock(m_event_mutex);

    // 找全部到时的任务, 
    auto begin = m_pending_events.lower_bound(event->m_arrive_time);
    auto end = m_pending_events.upper_bound(event->m_arrive_time);
    auto it = begin;

    for(it = begin; it != end; ++it)
    {
        // 找到对应的event跳出
        if(it->second == event)
        {
            DebugLog << "find timer event, now delete it. src arrive time=" << event->m_arrive_time;
            break;
        }
    }
    // 时间对了，对应事件删除
    if(it != m_pending_events.end())
    {
        m_pending_events.erase(it);
    }
    lock.unlock();

    DebugLog << "del timer event succ, origin arrvite time=" << event->m_arrive_time;
};

// 重新设置定时到达时间
void Timer::resetArriveTime()
{
    RWMutex::ReadLock lock(m_event_mutex);
    std::multimap<int64_t, TimerEvent::ptr> tmp = m_pending_events;
    lock.unlock();
 
    if(tmp.size() == 0)
    {
        DebugLog << "no timerevent pending, size = 0";
        return;
    }

    int64_t now = getNowMs();
    auto it = tmp.rbegin(); 
    if((*it).first < now) // 所有的定时器事件都发生了，没有事件能被重置了
    {   
        DebugLog << "all timer events has already expire";
        return;
    }

    int64_t interval = (*it).first - now;

    itimerspec new_value; // 定时器类型，有开始和间隔
    memset(&new_value, 0, sizeof(new_value));

    timespec ts;
    memset(&ts, 0, sizeof(ts));
    ts.tv_sec = interval / 1000;
    ts.tv_nsec = (interval % 1000) * 1000000;
    // 设置开始时间
    new_value.it_value = ts;
    // 设置定时器，放在m_fd
    int rt = timerfd_settime(m_fd, 0, &new_value, nullptr);

    if(rt != 0)
    {
        ErrorLog << "timer_settime error, interval =" << interval;
    }
    else
    {

    }
}

// 拿出到时的所有定时器，在定时器容器中保存删除，执行
void Timer::onTimer()
{
    char buf[8];
    while(1)
    {
        // EAGAIN 非阻塞read没有数据可读时，设置这个错误，提示下次再读，但是不阻塞
        // 读定时器事件
        if((g_sys_read_fun(m_fd, buf, 8) == -1) && errno == EAGAIN)
            break;
    }

    int64_t now = getNowMs();
    RWMutex::WriteLock lock(m_event_mutex);

    auto it = m_pending_events.begin();
    // 1. 拿出到时的事件
    std::vector<TimerEvent::ptr> tmps;
    std::vector<std::pair<int64_t, std::function<void()>>> tasks;

    // std::multimap<int64_t, TimerEvent::ptr> m_pending_events;
    for(it; it != m_pending_events.end(); ++it)
    {
        // 判断到时任务,并且没有被取消的任务
        if((*it).first <= now && !((*it).second->m_is_cancled))
        {
            // 保存到时的TimerEvent
            tmps.emplace_back((*it).second);
            // 保存到时的时间和任务
            tasks.emplace_back(std::make_pair((*it).second->m_arrive_time, (*it).second->m_task));
        }
        else
            break;  // 用的事multimap， 根据时间排序的，到某个时间不符合后面肯定也不符合
    }

    // 2. 删除到时的事件
    m_pending_events.erase(m_pending_events.begin(), it);
    
    lock.unlock();// 后面没有写行为，解锁

    // 3. 判断任务是否要再重复加入定时器定时执行, 是就再加入定时器事件
    for(auto it : tmps)
    {
        if(it->m_is_repeated)
            it->resetTime();
        
        addTimerEvent(it, false);
    }

    resetArriveTime(); // 

    // 4.执行到时任务
    for(auto it : tasks)
    {
        it.second();
    }
}




  
}; // namespace tinyrp
