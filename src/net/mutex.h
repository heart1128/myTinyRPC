#ifndef SRC_NET_MUTEX_H
#define SRC_NET_MUTEX_H

#include <pthread.h>
#include <memory>
#include <queue>

#include "src/coroutine/coroutine.h"

// sylar的原版文件

namespace tinyrpc{


template<class T>
struct ScopedLockImpl {// 局部锁
public:
    ScopedLockImpl(T &mutex)
        : m_mutex(mutex)
    {
        m_mutex.lock();
        m_locked = true;
    }
    ~ScopedLockImpl()
    {
        unlock();
    }

public:
    void lock()
    {
        // 没锁就上锁
        if(!m_locked)
        {
            m_mutex.lock();
            m_locked = true;
        }
    }

    void unlock()
    {
        if(m_locked)
        {
            m_mutex.unlock();
            m_locked = false;
        }
    }

private:
    // 锁
    T& m_mutex;
    // 是否已经上锁
    bool m_locked;
};

/**
 * 
 * @brief 局部读锁模板实现
 * 
 */
template<class T>
struct ReadScopedLockImpl{
public:
    ReadScopedLockImpl(T& mutex)
    : m_mutex(mutex)
    {
        m_mutex.rdlock();
        m_locked = true;
    }
    ~ReadScopedLockImpl()
    {
        unlock();
    }

public:
    void lock()
    {
        if(!m_locked)
        {
            m_mutex.rdlock();
            m_locked = true;
        }
    }
    void unlock()
    {
        if(m_locked)
        {
            m_mutex.unlock();
            m_locked = false;
        }
    }

private:
    T& m_mutex;
    bool m_locked;
};
/**
 * 
 * @brief 局部写锁模板实现
 * 
 */
template<class T>
struct WriteScopedLockImpl{
public:
    WriteScopedLockImpl(T& mutex)
    : m_mutex(mutex)
    {
        m_mutex.wrlock();
        m_locked = true;
    }
    ~WriteScopedLockImpl()
    {
        unlock();
    }

public:
    void lock()
    {
        if(!m_locked)
        {
            m_mutex.wrlock();
            m_locked = true;
        }
    }
    void unlock()
    {
        if(m_locked)
        {
            m_mutex.unlock();
            m_locked = false;
        }
    }

private:
    T& m_mutex;
    bool m_locked;
};


/*

------------ 锁实现
使用的是linux的独有锁，应该可以改成c++11的锁

*/
class Mutex{
public:
    // 局部锁
    typedef ScopedLockImpl<Mutex> Lock;

    Mutex()
    {
        // 初始化线程锁
        pthread_mutex_init(&m_mutex, nullptr);
    }

    ~Mutex()
    {
        // 销毁锁
        pthread_mutex_destroy(&m_mutex);
    }

    // 上锁
    void lock()
    {
        pthread_mutex_lock(&m_mutex);
    }

    // 解锁
    void unlock()
    {
        pthread_mutex_unlock(&m_mutex);
    }

    // 获取锁
    pthread_mutex_t* getMutex()
    {
        return &m_mutex;
    }


private:
    pthread_mutex_t m_mutex; // 线程锁
};

/*

------------ 读写锁
当读写锁是写加锁状态时：在这个锁被解锁之前，所有试图对这个锁加锁的线程都会阻塞（不论是读还是写）
当读写锁是读加锁状态时：所有试图以读模式对它进行加锁的线程都可以得到访问权。但是任何希望以写模式对此锁进行加锁的线程都会阻塞，直到所有的线程释放它们的读锁为止
在高并发下效率更高，可以单独控制读写，互斥锁只能全部锁住，不能多个线程一起读
pthread_rwlock_t
*/
class RWMutex{
public:
    // 局部读写锁接口
    typedef ReadScopedLockImpl<RWMutex> ReadLock;
    typedef WriteScopedLockImpl<RWMutex> WriteLock;

public:
    RWMutex()
    {
        pthread_rwlock_init(&m_lock, nullptr);
    }
    
    ~RWMutex()
    {
        pthread_rwlock_destroy(&m_lock);
    }
    // 读模式读锁，所有线程可读不可写
    void rdlock()
    {
        pthread_rwlock_rdlock(&m_lock);
    }
    // 写锁
    void wrlock()
    {
        pthread_rwlock_wrlock(&m_lock);
    }
    // 解锁
    void unlock()
    {
        pthread_rwlock_unlock(&m_lock);
    }

private:
    pthread_rwlock_t m_lock;
};

/*

----------- 读写协程锁

*/
class CoroutineMutex{
public:
    typedef ScopedLockImpl<CoroutineMutex> Lock;

public:
    CoroutineMutex();
    ~CoroutineMutex();

    void lock();
    void unlock();
private:
    bool m_lock {false};
    Mutex m_mutex;
    std::queue<Coroutine*> m_sleep_cors; // 加入队列的都是等待唤醒的协程

};

} // namespace tinyrp




#endif