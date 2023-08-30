#include <vector>

#include "src/coroutine/coroutine_pool.h"
#include "src/coroutine/coroutine.h"
#include "src/comm/config.h"
#include "src/comm/log.h"


namespace tinyrpc{

extern tinyrpc::Config::ptr gRpcConfig;

// 全局的协程池容器
static CoroutinePool* t_coroutine_container_ptr = nullptr;

// 获取协程池对象
CoroutinePool* getCoroutinePool()
{
    if(!t_coroutine_container_ptr)
        t_coroutine_container_ptr = new CoroutinePool(gRpcConfig->m_cor_pool_size, gRpcConfig->m_cor_stack_size);

    return t_coroutine_container_ptr;
}


/*

    *********** CoroutinePool ***********

*/
// 初始化协程池，创建协程，并且给栈空间和sp指针
CoroutinePool::CoroutinePool(int pool_size, int stack_size /*1024 * 128*/)
: m_pool_size(pool_size),
  m_stack_size(stack_size)
{
    // 设置主协程,函数里如果当前携程为空就创建一个空协程为主协程，设置为当前协程。
    Coroutine::getCurrentCoroutine(); 

    // 一个位置一条内存列表，整个vector就是内存池, pool_size就是块数量
    m_memory_pool.push_back(std::make_shared<Memory>(stack_size, pool_size));

    Memory::ptr tmp = m_memory_pool[0];  // 随便找的一个对象，为了调用

    for(int i = 0; i < pool_size; ++i)
    {
        // 给一个栈大小和栈指针，也就是获取的堆的指针
        Coroutine::ptr cor = std::make_shared<Coroutine>(stack_size, tmp->getBlock());
        cor->setIndex(i);  // 协程在协程池中的index
        m_free_cors.push_back(std::make_pair(cor, false));  // false表示还未被使用
    }
}

CoroutinePool::~CoroutinePool(){}

// 获取一个协程池的实例
Coroutine::ptr CoroutinePool::getCoroutineInstanse()
{
    Mutex::Lock lock(m_mutex);
    // 1. 首先找已经创建的但是处于睡眠的协程使用
    // 因为已经使用过的协程是经过写入的，已经创建了
    for(int i = 0; i < m_pool_size; ++i)
    {
        // 找到一个不在协程调度的第二阶段也就是不在COFunc中的协程，并且未被使用的协程
        if(!m_free_cors[i].first->getIsInCoFunc() && !m_free_cors[i].second)
        {
            m_free_cors[i].second = true;
            Coroutine::ptr cor = m_free_cors[i].first;  // 取出这个协程，拿出使用
            lock.unlock();

            return cor;
        }
    }

    // 2. 如果没有已经创建使用过的协程，只能创建一个，指定栈空间
    // 在内存池中找到第一个空闲的块，创建一个协程
    // 之后创建的协程不加入free_cors。用完直接返还内存
    for(size_t i = 1; i < m_memory_pool.size(); ++i)
    {
        char* tmp = m_memory_pool[i]->getBlock();
        if(tmp)
        {
            Coroutine::ptr cor = std::make_shared<Coroutine>(m_stack_size, tmp);
            return cor;
        }
    }

    // 3. 如果分配的m_memory_pool已经满了，就再添加一个块
     // 返回最后一个新分配的块创建的协程栈的协程
    m_memory_pool.push_back(std::make_shared<Memory>(m_stack_size, m_pool_size));
    return std::make_shared<Coroutine>(m_stack_size, m_memory_pool[m_memory_pool.size() - 1]->getBlock());
}

// 归还协程
void CoroutinePool::returnCoroutine(Coroutine::ptr cor)
{
    // 协程index是在初始化协程池的时候创建了m_pool_size个协程的编号，pool_size就是一个Memory有多少块的数量
    // 当时是加入到m_free_cors中的，设置false就能归还
    int i = cor->getIndex();
    // 如果是在
    if(i >= 0 && i < m_pool_size)
    {
        m_free_cors[i].second = false;
    }
    else
    {
        // 不是在m_free_cors中的，也就是之后再创建但是没有加入到m_free_cors中的直接返还内存
        // 也就是说常驻的协程池中的协程数量就是m_pool_size个，其他的都是用时创建，不用时直接归还。
        for(size_t i = 1; i < m_memory_pool.size(); ++i)
        {
            if(m_memory_pool[i]->hasBlock(cor->getStackPtr()))
            {
                m_memory_pool[i]->backBlock(cor->getStackPtr());
            }
        }
    }
}

}