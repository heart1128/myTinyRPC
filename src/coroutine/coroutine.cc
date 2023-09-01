#include <atomic>
#include <cstring>

#include "src/coroutine/coroutine.h"
#include "src/comm/run_time.h"
#include "src/comm/config.h"
#include "src/comm/log.h"

namespace tinyrpc{

// 主协程，所有的io线程都有一个主协程，和线程共享栈
static thread_local Coroutine* t_main_coroutine = NULL;

// 当前线程运行的协程
static thread_local Coroutine* t_cur_coroutine = NULL;

// 当前协程的运行时间
static thread_local RunTime* t_cur_run_time = NULL;

static std::atomic_int t_coroutine_count {0};   // 运行的协程数量
static std::atomic_int t_cur_coroutine_id {1};  // 当前的协程id


// 获取当前协程id
int getCoroutineIndex()
{
    return t_cur_coroutine_id;
}
//  获取当前协程的运行时间
RunTime* getCurrentRunTime()
{
    return t_cur_run_time;
}


// 主协程切换到协程后，调用回调函数，再切换到回调函数栈中
void CoFunction(Coroutine* co)
{
    if(co != nullptr)
    {
        // 设置在协程函数中
        co->setIsInCoFunc(true);

        // 调用回调函数，协程执行
        co->m_call_back();

        // 退出协程函数
        co->setIsInCoFunc(false);
    }

    // 协程已经调用回调函数，任务完成了
    // 回调函数完成了栈就到底了，所以要使用挂起切换到主协程中。
    Coroutine::Yield();
}


// 主协程使用的构造函数，不需要设置栈，和线程使用用一个栈
Coroutine::Coroutine()
{
    // 主协程的协程id设为0
    m_cor_id = 0;
    t_coroutine_count++;
    // 初始化上下文结构体
    memset(&m_coctx, 0, sizeof(m_coctx));
    t_cur_coroutine = this;
}


// 子协程需要设置栈， 不能申请栈，所以申请的是堆空间
Coroutine::Coroutine(int size, char* stack_ptr)
: m_stack_size(size),
  m_stack_sp(stack_ptr)
{
    assert(stack_ptr);


    if(!t_main_coroutine)
        t_main_coroutine = new Coroutine();

    m_cor_id = t_cur_coroutine_id++;
    t_coroutine_count++;
}

// 有回调函数的构造, 子协程执行回调函数
Coroutine::Coroutine(int size, char* stack_ptr, std::function<void()> cb)
: m_stack_size(size),
  m_stack_sp(stack_ptr)
{
    assert(m_stack_sp);
    if(!t_main_coroutine)
        t_main_coroutine = new Coroutine();

    setCallBack(cb);
    m_cor_id = t_cur_coroutine_id++;
    t_coroutine_count++;
}

bool Coroutine::setCallBack(std::function<void()> cb)
{
    // 主协程不负责执行回调函数任务
    if(this == t_main_coroutine)
    {
        ErrorLog << "main coroutine can't set callback";
        return false;
    }
    // 当前协程正在执行cofunc中，要等待协程挂起
    if(m_is_in_cofunc)
    {
        ErrorLog << "this coroutine is in CoFunction";
        return false;
    }

    m_call_back = cb;
    // 栈是自大到小地址的，而stack_sp分配的堆是自小到大的，所以要转换下，将top放到最高位，之后的汇编会一直往下走
    char* top = m_stack_sp + m_stack_size;
    // 内存对齐，在64位机器下每次存储8位，所以要8位对齐
    // -16LL其实就是低4位都是0，高位都是1的数，进行&操作top就最后4位都是0就是8的倍数了，进行了8的内存对齐
    top = reinterpret_cast<char*>((reinterpret_cast<unsigned long>(top)) & -16LL);

    memset(&m_coctx, 0, sizeof(m_coctx));

    m_coctx.regs[kRSP] = top; // 初始栈顶和栈底是同一个位置
    m_coctx.regs[kRBP] = top;
    m_coctx.regs[kRETAddr] = reinterpret_cast<char*>(CoFunction); // 切换下一个地址到执行协程调用回调函数的函数地址，下一次就进入了协程调度切换中
    m_coctx.regs[kRDI] = reinterpret_cast<char*>(this); // 第一个参数的位置，把this传入CoFunction调用成员cb函数

    m_can_resume = true; // 协程上下文设置好了，设置为可以唤醒

    return true;
}

Coroutine::~Coroutine()
{
    t_coroutine_count--; // 协程析构，协程池--
}

// 获取当前协程，如果没有子协程，就返回一个主协程，也就是当前的线程
Coroutine* Coroutine::getCurrentCoroutine()
{
    if(t_cur_coroutine == nullptr)
    {
        t_main_coroutine = new Coroutine();
        t_cur_coroutine = t_main_coroutine;
    }

    return t_cur_coroutine;
}

// 返回主协程，没有就创建
Coroutine* Coroutine::getMainCoroutine()
{
    if(t_main_coroutine)
    {
        return t_main_coroutine;
    }
    
    t_main_coroutine = new Coroutine();

    return t_main_coroutine;
}

// 判断当前协程是不是主协程
bool Coroutine::isMainCoroutine()
{
    if(t_main_coroutine == nullptr || t_cur_coroutine == t_main_coroutine)
        return true;
    
    return false;
}


// 协程完成任务，切换回主协程
void Coroutine::Yield()
{   
    // 没有主协程
    if(t_main_coroutine == nullptr)
    {
        ErrorLog << "main coroutine is nullptr !";
        return;
    }
    // 当前协程就是主协程，不需要切换
    if(t_cur_coroutine == t_main_coroutine)
    {
        ErrorLog << "current coroutine is main coroutine";
        return;
    }

    // 切换到主协程
    Coroutine* co = t_cur_coroutine;
    t_cur_coroutine = t_main_coroutine; // 切换回主协程就，当前协程就变回了主协程
    t_cur_run_time = NULL;
        // 第一个是当前协程，第二个是主协程
    coctx_swap(&(co->m_coctx), &(t_main_coroutine->m_coctx));

}

// 唤醒协程，主协程 -> 子协程
void Coroutine::Resume(Coroutine* co)
{
        // 当前协程不是主协程报错，因为是非对称协程，不能任意两个协程之前切换
    if (t_cur_coroutine != t_main_coroutine) 
    {
        ErrorLog << "swap error, current coroutine must be main coroutine";
        return;
    }
        // 主协程不存在
    if(!t_main_coroutine)
    {
        ErrorLog << "main coroutine is nullptr";
        return;
    }
        // co协程不存或者不能被唤醒的状态
    if(!co || !co->m_can_resume)
    {
        ErrorLog << "pending coroutine is nullptr or can_resume is false";
        return;
    }
        // 当前协程就是co，不需要切换
    if(t_cur_coroutine == co)
    {
        DebugLog << "current coroutine is pending cor, need't swap";
        return;
    }

    // 进行切换
    t_cur_coroutine = co;
    t_cur_run_time = co->getRunTime();
    // 主 -> 子
    coctx_swap(&(t_main_coroutine->m_coctx), &(co->m_coctx));
}

}