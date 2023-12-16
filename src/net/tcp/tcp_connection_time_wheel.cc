#include "tcp_connection_time_wheel.h"
#include <queue>
#include <vector>
#include "src/net/tcp/abstract_slot.h"
#include "src/net/tcp/tcp_connection.h"
#include "src/net/timer.h"

namespace tinyrpc {

tinyrpc::TcpTimeWheel::TcpTimeWheel(Reactor *reactor, int bucket_count, int inteval /*=10*/)
: m_reactor(reactor), m_bucket_count(bucket_count), m_inteval(inteval)
{
    // 创建时间轮，插入空槽
    for(int i = 0; i < bucket_count; ++i)
    {
        std::vector<TcpConnectionSlot::ptr> tmp;
        m_wheel.push(tmp);
    }

    // 设置定时器事件，定时唤醒删除槽
    m_timer_event = std::make_shared<TimerEvent>(m_inteval * 1000, true, std::bind(&TcpTimeWheel::loopFunc, this));
    // reactor中保存一个定时器，不断往里添加定时器事件，会注册到reactor中
    m_reactor->getTimer()->addTimerEvent(m_timer_event);

}

// 删除定时器事件就行
TcpTimeWheel::~TcpTimeWheel()
{
    m_reactor->getTimer()->delTimerEvent(m_timer_event);
}

// 加入定时连接，插入队尾的bucket就行
void TcpTimeWheel::fresh(TcpConnectionSlot::ptr slot)
{
    DebugLog << "fresh connection";
    m_wheel.back().emplace_back(slot);
}
// 删除队头，添加空槽
void TcpTimeWheel::loopFunc()
{
    m_wheel.pop();
    std::vector<TcpConnectionSlot::ptr> tmp;
    m_wheel.push(tmp);
}

} // namespace tinyrpc

