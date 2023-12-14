#include <memory>
#include <map>
#include <time.h>
#include <stdlib.h>
#include <semaphore.h>
#include "src/net/reactor.h"
#include "src/net/tcp/io_thread.h"
//#include "src/net/tcp/tcp_connection.h"
#include "src/net/tcp/tcp_server.h"
#include "src/net/tcp/tcp_connection_time_wheel.h"
#include "src/coroutine/coroutine.h"
#include "src/coroutine/coroutine_pool.h"
#include "src/comm/config.h"

namespace tinyrpc {

extern tinyrpc::Config::ptr gRpcConfig;

// 线程周期变量，线程结束变量销毁 c++11
static thread_local Reactor* t_reactor_ptr = nullptr;
static thread_local IOThread* t_cur_io_thread = nullptr;


// 初始化信号量，创建线程，等待init信号量
IOThread::IOThread()
{
    // pshared不为０时此信号量在进程间共享，否则只能为当前进程的所有线程共享；value给出了信号量的初始值。　
    int rt = sem_init(&m_init_semaphore, 0, 0);
    assert(rt == 0);

    rt = sem_init(&m_start_semaphore, 0, 0);
    assert(rt == 0);

    // 创建线程,main
    // 线程号，线程属性，执行函数，传入参数
    pthread_create(&m_thread, nullptr, &IOThread::main, this);

    // 等待main完成初始化，这里的信号量才会不阻塞
    DebugLog << "semaphore begin to wait until new thread finish IOThread::main() to init";
    rt = sem_wait(&m_init_semaphore);
    assert(rt == 0);
    DebugLog << "semaphore wait end, finish create io thread";

    sem_destroy(&m_init_semaphore);
}

IOThread::~IOThread()
{
    m_reactor->stop();
    // 会阻塞当前进程
    pthread_join(m_thread, nullptr);

    if(m_reactor != nullptr)
    {
        delete m_reactor;
        m_reactor = nullptr;
    }
}

IOThread* IOThread::getCurrentIOThread() 
{
  return t_cur_io_thread;
}

sem_t* IOThread::getStartSemaphore() 
{
  return &m_start_semaphore;
}

Reactor* IOThread::getReactor() 
{
  return m_reactor;
}

pthread_t IOThread::getPthreadId() 
{
  return m_thread;
}

void IOThread::setThreadIndex(const int index) 
{
  m_index = index;
}

int IOThread::getThreadIndex() 
{
  return m_index;
}

//设置参数，等待start信号量执行reactor->loop，信号量在iothread_pool的start中Post
void* IOThread::main(void* arg)
{
    t_reactor_ptr = new Reactor();  // 这是一个线程局部变量，一个线程只能有一个reactor
    assert(t_reactor_ptr != NULL);

    IOThread* thread = static_cast<IOThread*>(arg); //传入的是IOThrad this
    t_cur_io_thread = thread; // thread_local

    thread->m_reactor = t_reactor_ptr;
    thread->m_reactor->setReactorType(SubReactor);  // 子reactor处理io
    thread->m_tid = gettid();

    Coroutine::getCurrentCoroutine(); // 不要返回值，就是为了如果当前协程不存在，创建一个新的

    DebugLog << "finish iothread init, now post semaphore";
    sem_post(&thread->m_init_semaphore);  // 初始化完成

    sem_wait(&thread->m_start_semaphore);  // 等待pool的start唤醒信号量 

    sem_destroy(&thread->m_start_semaphore);

    DebugLog << "IOThread " << thread->m_tid << " begin to loop"; // 唤醒之后执行Loop
    t_reactor_ptr->loop();

    return nullptr;
}


/**
 * 
 * 
 *          IO Thread pool
 * 
 * 
*/

// 初始化，创建IOThread加入线程池
IOThreadPool::IOThreadPool(int size)
: m_size(size)
{
    for(int i = 0; i < size; ++i)
    {
        m_io_threads.emplace_back(std::make_shared<IOThread>());
        m_io_threads[i]->setThreadIndex(i);
    }
}

// 唤醒start信号量，执行reactor->loop
void IOThreadPool::start()
{
    for(int i = 0; i < m_size; ++i)
    {
        int rt = sem_post(m_io_threads[i]->getStartSemaphore());
        assert(rt == 0);
    }
}

// 拿一个没用过的线程
IOThread* IOThreadPool::getIOThread()
{
    if(m_index == m_size || m_index == -1)
        m_index = 0;
    
    return m_io_threads[m_index].get();
}

int IOThreadPool::getIOThreadPoolSize() 
{
  return m_size;
}

void IOThreadPool::broadcastTask(std::function<void()> cb) 
{
  for (auto i : m_io_threads) {
    // reactor加入到m_pendding_task队列中，在loop的时候不断执行cb
    i->getReactor()->addTask(cb, true);
  }
}

void IOThreadPool::addTaskByIndex(int index, std::function<void()> cb) 
{
  if (index >= 0 && index < m_size) 
  {
    m_io_threads[index]->getReactor()->addTask(cb, true);
  }
}

// 随机添加协程到线程，一般不添加到当前线程
void IOThreadPool::addCoroutineToRandomThread(Coroutine::ptr cor, bool self /* = false*/)
{
    // 只有一个线程，指定
    if(m_size == 1)
    {
        m_io_threads[0]->getReactor()->addCoroutine(cor, true); // 添加协程，立即唤醒epoll_wait,执行协程唤醒
        return;
    }

    srand(time(0));
    int i = 0;
    while(1)
    {
        i = rand() % (m_size);
        // 不放在当前线程，但是随机到了当前线程，就++，如果++溢出了就-2找前面一个
        if (!self && m_io_threads[i]->getPthreadId() == t_cur_io_thread->getPthreadId()) 
        {
            i++;
            if (i == m_size) 
            {
                i -= 2;
            }
        }
        break;
    }

    // 随机线程
    m_io_threads[i]->getReactor()->addCoroutine(cor, true);
}

// 拿一个之前使用过的协程实例，设置cb加入线程中，唤醒协程执行cb
Coroutine::ptr IOThreadPool::addCoroutineToRandomThread(std::function<void()> cb, bool self/* = false*/) 
{
  Coroutine::ptr cor = getCoroutinePool()->getCoroutineInstanse();
  cor->setCallBack(cb);
  addCoroutineToRandomThread(cor, self);
  return cor;
}

Coroutine::ptr IOThreadPool::addCoroutineToThreadByIndex(int index, std::function<void()> cb, bool self/* = false*/) 
{
  if (index >= (int)m_io_threads.size() || index < 0) 
  {
    ErrorLog << "addCoroutineToThreadByIndex error, invalid iothread index[" << index << "]";
    return nullptr;
  }
  Coroutine::ptr cor = getCoroutinePool()->getCoroutineInstanse();
  cor->setCallBack(cb);
  m_io_threads[index]->getReactor()->addCoroutine(cor, true);
  return cor;

}

void IOThreadPool::addCoroutineToEachThread(std::function<void()> cb) 
{
  for (auto i : m_io_threads) 
  {
    Coroutine::ptr cor = getCoroutinePool()->getCoroutineInstanse();
    cor->setCallBack(cb);
    i->getReactor()->addCoroutine(cor, true);
  }
}

    
} // namespace tinyrpc
