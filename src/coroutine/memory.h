#ifndef SRC_COROUTINE_MEMORY_H
#define SRC_COROUTINE_MEMORY_H

#include <memory>
#include <atomic>
#include <vector>

#include "src/net/mutex.h"


namespace tinyrpc{

class Memory{
public:
    typedef std::shared_ptr<Memory> ptr;

public:
    Memory(int block_size, int block_count);
    ~Memory();

public:
    // 
    int getRefCount();
    //
    char* getStart();
    // 
    char* getEnd();
    //
    char* getBlock();
    //
    void backBlock(char* s);
    //
    bool hasBlock(char* s);

private:
    int m_block_size {0};           // 内存块大小
    int m_block_count {0};          // 内存块的总量

    int m_size {0};              // 总量大小 
    char* m_start {NULL};       // 指向开始
    char* m_end {NULL};         // 指向结尾

    std::atomic<int> m_ref_counts {0};      // 内存块被引用的总量
    std::vector<bool> m_blocks;             // 块被使用的标志，这个vector<bool>是否 有问题？
    Mutex m_mutex;
};



} // namespace tinyrpc


#endif