#ifndef SRC_COROUTINE_COROUTIN_H
#define SRC_COROUTINE_COROUTIN_H

#include <memory>
#include <functional>

#include "src/comm/run_time.h"
#include "src/coroutine/coctx.h"

namespace tinyrpc{

// ------------- 辅助函数
int getCoroutineIndex();

RunTime* getCurrentRunTime();

void setCurrentRunTime(RunTime* v);


// *********************协程类*********************
class Coroutine{
public:
    typedef std::shared_ptr<Coroutine> ptr;

private:
    Coroutine();

public:
    Coroutine(int size, char* stack_ptr);

    Coroutine(int size, char* stack_ptr, std::function<void()> cb);

    ~Coroutine();

public:
    static bool isMainCoroutine();
    bool setCallBack(std::function<void()> cb);

    int getCorId() const 
    {
        return m_cor_id; 
    }
    void setIsInCoFunc(const bool v) 
    {
        m_is_in_cofunc = v;
    }

    bool getIsInCoFunc() const 
    {
        return m_is_in_cofunc;
    }

    std::string getMsgNo() 
    {
        return m_msg_no;
    }

    RunTime* getRunTime() 
    {
        return &m_run_time; 
    }

    void setMsgNo(const std::string& msg_no) 
    {
        m_msg_no = msg_no;
    }

    void setIndex(int index) 
    {
        m_index = index;
    }

    int getIndex() 
    {
        return m_index;
    }

    char* getStackPtr() 
    {
        // stack_sp指针在内存池中的一段指向本协程的块头指针
        return m_stack_sp;
    }

    int getStackSize() 
    {
        return m_stack_size;
    }

    void setCanResume(bool v) 
    {
        m_can_resume = v;
    }

public:
    // 挂起协程其实就是从当前协程切换到主协程，此时当前协程让出CPU，主协程抢占CPU，栈空间恢复为原来线程的栈，执行主协程的代码。
    // 目的就是为了切换回主协程
    static void Yield();
    //唤醒协程其实就是从主协程切换到目标协程，此时主协程让出CPU，目标协程抢占CPU，然后在目标协程的空间上执行目标协程的代码。
    // 必须在当前协程是主协程的情况下才行唤醒其他目标协程。
    static void Resume(Coroutine* cor);

    static Coroutine* getCurrentCoroutine();
    static Coroutine* getMainCoroutine();


public:
    std::function<void()> m_call_back;   // 回调函数

private:
    int m_cor_id {0};     // 协程id
    coctx m_coctx;          // 协程寄存器结构体
    int m_stack_size {0};  // 协程栈结构体大小
    char* m_stack_sp {NULL}; // 协程栈空间，用malloc或者mmap初始化。
    bool m_is_in_cofunc {false}; // 切换到协程函数中就是true， 协程函数结束调用回调函数之后就是false
    std::string m_msg_no;
    RunTime m_run_time;

    bool m_can_resume {true};  // 标识能够唤醒协程

    int m_index {-1};   // 当前协程在协程池中的index
};

}

#endif