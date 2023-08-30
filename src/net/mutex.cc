#include <pthread.h>
#include <memory>

#include "src/coroutine/coroutine.h"
#include "src/comm/log.h"
#include "src/net/mutex.h"


namespace tinyrpc{

CoroutineMutex::CoroutineMutex(){}

CoroutineMutex::~CoroutineMutex()
{
    if(m_lock)
        unlock();
}

void CoroutineMutex::lock()
{   
    // 主协程不能使用锁
    if(Coroutine::isMainCoroutine())
    {
        ErrorLog << "main coroutine can't use coroutine mutex";
        return;
    }

    Coroutine* cor = Coroutine::getCurrentCoroutine();

    Mutex::Lock lock(m_mutex);  // 创建对象构造函数已经上锁了

    if(!m_lock) // 改下上锁的状态
    {
        m_lock = true;
        DebugLog << "coroutine succ get coroutine mutex";
        lock.unlock(); 
    }
    else
    {
        m_sleep_cors.push(cor);
        auto tmp = m_sleep_cors;
        lock.unlock();

        DebugLog << "coroutine yield, pending coroutine mutex, current sleep queue exist ["
            << tmp.size() << "] coroutines";

        Coroutine::Yield();  // 协程挂起
    }
}

void CoroutineMutex::unlock()
{
    if(Coroutine::isMainCoroutine())
    {
        ErrorLog << "main coroutine can't use coroutine mutex";
        return;
    }

    Mutex::Lock lock(m_mutex);
    if(m_lock)
    {
        m_lock = false;
        if(m_sleep_cors.empty())
            return;
        
        Coroutine* cor = m_sleep_cors.front();
        m_sleep_cors.pop();
        lock.unlock();

        if(cor)
        {
            // 唤醒睡眠队列中的第一个协程
            DebugLog << "coroutine unlock, now to resume coroutine[" << cor->getCorId() << "]";

            // Reactor框架加入到协程的任务，未实现
        }
    }

}
    
} // namespace tinyrpc
