#ifndef SRC_COROUTINE_COROUTINE_POOL_H
#define SRC_COROUTINE_COROUTINE_POOL_H

#include <vector>

#include "src/coroutine/coroutine.h"
#include "src/net/mutex.h"
#include "src/coroutine/memory.h"


namespace tinyrpc{

// 协程池是所有线程共用的，也就是 m:n协程池模型，m个线程对n个协程自由调用
// 如果使用1:n也就是每个线程使用单独的协程池，就会出现如果io处理时间过长，其他的请求阻塞，因为就一个线程
class CoroutinePool{
public:
    CoroutinePool(int pool_size, int stack_size = 1024 * 128);
    ~CoroutinePool();

    Coroutine::ptr getCoroutineInstanse();  // 获取一个协程

    void returnCoroutine(Coroutine::ptr cor);  // 归还一个协程

private:
    int m_pool_size {0};
    int m_stack_size {0};

    // bool 判断协程是否被使用
    std::vector<std::pair<Coroutine::ptr, bool>> m_free_cors; // 协程池中的协程

    // 锁
    Mutex m_mutex;

    std::vector<Memory::ptr> m_memory_pool;
};

CoroutinePool* getCoroutinePool();

}


#endif