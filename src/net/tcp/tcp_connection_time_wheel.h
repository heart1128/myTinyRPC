#ifndef SRC_NET_TCP_TCPCONNECTIONTIMEWHEEL_H
#define SRC_NET_TCP_TCPCONNECTIONTIMEWHEEL_H

#include "src/net/timer.h"
#include <queue>
#include <vector>
#include "src/net/tcp/abstract_slot.h"
#include "src/net/reactor.h"


namespace tinyrpc{

class TcpConnection;

class TcpTimeWheel{
public:
    typedef std::shared_ptr<TcpTimeWheel> ptr;

    typedef AbstractSlot<TcpConnection> TcpConnectionSlot;

    TcpTimeWheel(Reactor* reactor, int bucket_count, int inteval = 10);

    ~TcpTimeWheel();

    void fresh(TcpConnectionSlot::ptr slot);

    void loopFunc();

private:
    Reactor* m_reactor {nullptr};
    int m_bucket_count {0};
    int m_inteval {0};  // 时间间隔

    TimerEvent::ptr m_timer_event;  //时间轮的定时器事件，用于定时删除一个槽，添加一个空槽
    std::queue<std::vector<TcpConnectionSlot::ptr>> m_wheel;

};
    
} // namespace tinyrpc



#endif