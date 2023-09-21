#ifndef SRC_NET_TCP_IO_THREAD_H
#define SRC_NET_TCP_IO_THREAD_H

#include <memory>  // shaerd_ptr
#include <map>
#include <atomic>
#include <functional>
#include <semaphore.h>
#include "src/net/reactor.h"
// #include "src/net/tcp/tcp_connection_time_wheel.h"
#include "src/coroutine/coroutine.h"

namespace tinyrpc {

class TcpServer;

class IOThread{

public:
    using ptr = std::shared_ptr<IOThread>;

public:
    IOThread();
    ~IOThread();

public:

    Reactor* getReactor();

    // void addClient(TcpConnection* tcp_conn);

    pthread_t getPthreadId();

    void setThreadIndex(const int index);

    int getThreadIndex();

    sem_t* getStartSemaphore();

public:
    static IOThread* getCurrentIOThread();  // 单例

private:
    static void* main(void* arg);

private:
    Reactor* m_reactor {nullptr};
    pthread_t m_thread {0};
    pid_t m_tid {-1};
    // TimerEvent::ptr m_timer_event {nullptr};

    int m_index {-1};

    sem_t m_init_semaphore;   // 初始化信号量，用于等待唤醒初始化

    sem_t m_start_semaphore; // 等待唤醒执行reactor的loop函数

};


class IOThreadPool {

 public:
  typedef std::shared_ptr<IOThreadPool> ptr;

  IOThreadPool(int size);

  void start();

  IOThread* getIOThread();

  int getIOThreadPoolSize();

  void broadcastTask(std::function<void()> cb);  // 设置每个reactor的task都是cb

  void addTaskByIndex(int index, std::function<void()> cb);  // 指定thread设置

  void addCoroutineToRandomThread(Coroutine::ptr cor, bool self = false); // 随机加入协程

  // add a coroutine to random thread in io thread pool
  // self = false, means random thread cann't be current thread
  // please free cor, or causes memory leak
  // call returnCoroutine(cor) to free coroutine
  Coroutine::ptr addCoroutineToRandomThread(std::function<void()> cb, bool self = false); // 给协程设置cb之后随机加入线程

  Coroutine::ptr addCoroutineToThreadByIndex(int index, std::function<void()> cb, bool self = false); // 指定index

  void addCoroutineToEachThread(std::function<void()> cb);  // 每个协程指定相同任务，设置到每个thread中

 private:
  int m_size {0};

  std::atomic<int> m_index {-1};  // 记录当前用到了第几个线程

  std::vector<IOThread::ptr> m_io_threads;
  
};

    
} // namespace tinyrpc



#endif