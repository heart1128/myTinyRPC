#ifndef SRC_COROUTINE_COCTX_H
#define SRC_COROUTINE_COCTX_H

namespace tinyrpc{

enum{
    kRBP = 6,           // 在regs寄存器数组中的位置，rbp是栈底指针，位置是regs[6]
    kRDI = 7,           // rdi, 第一个参数的位置regs[7]
    kRSI = 8,           // rsim 第二个参数的位置regs[8]
    kRETAddr = 9,       // 下一个执行地址，会保存在rip中
    kRSP = 13           // 栈顶指针
};

// 协程上下文结构体
struct coctx
{
    void* regs[14];
};

// 协程切换函数，协程切换实质上就是原协程栈保存，换成需要切换的协程栈，只是涉及一些寄存器的保存和切换，使用汇编
extern "C"{
    // 第一个参数是当前的协程上下文结构体，第二个是需要切换的
    // 实现的是非对称协程，所以第一个永远是一个主协程。
    extern void coctx_swap(coctx *, coctx *) asm("coctx_swap");
};


}


#endif