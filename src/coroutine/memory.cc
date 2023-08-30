#include <memory>


#include "src/comm/log.h"
#include "src/coroutine/memory.h"

/**
 * 整个内存块是用过malloc申请一大块内存
 *  1. 拿取：设置block大小，每次拿一块，标记每块使用情况，拿没使用的块
 *  2. 归还：计算归还指针的位置，设置为未使用即可
 * 
*/

namespace tinyrpc{

Memory::Memory(int block_size, int block_count)
: m_block_size(block_size),
  m_block_count(block_count)
{
    // 总量
    m_size = m_block_size * m_block_count;
    // malloc分配内存块，start指向开端
    m_start = (char*)malloc(m_size);
    assert(m_start != (void*)-1);

    InfoLog << "succ mmap" << m_size << " bytes memory";

    m_end = m_start + m_size - 1; // 地址0开始
    m_blocks.resize(m_block_count); 

    for(size_t i = 0; i < m_blocks.size(); ++i) 
    {
        m_blocks[i] = false;
    }
    m_ref_counts = 0; // 被引用的块数量为0
}

Memory::~Memory()
{
    // 未分配或者分配失败
    if(!m_start || m_start == (void*)-1)
        return;
    
    free(m_start);
    InfoLog << "~succ free munmap " << m_size << " bytes memory";
    m_start = NULL;
    m_ref_counts = 0;

}

char* Memory::getStart()
{
    return m_start;
}

char* Memory::getEnd()
{
    return m_end;
}

int Memory::getRefCount()
{
    return m_ref_counts;
}

// 分配块出去
char* Memory::getBlock()
{
    int t = -1;
    // 上锁拿块，否则本协程准备拿i的块，但是没锁被其他 协程拿走了，两个协程都拿到了i块，出现数据错乱
    Mutex::Lock lock(m_mutex);
        // 拿取第一个没有被使用的块
    for(size_t i = 0; i < m_blocks.size(); ++i)
    {
        if(m_blocks[i] == false)
        {
            m_blocks[i] = true;
            t = i;
            break;
        }
    }

    lock.unlock();
        // 没有空闲块
    if(t == -1)
        return NULL;
    
    m_ref_counts++;
    // 返回第i块的首地址
    return m_start + (t * m_block_size);
}

// 归还块
void Memory::backBlock(char* s)
{
    if(s > m_end || s < m_start)
    {
        ErrorLog << "error, this block is not belong to this Memory";
        return;
    }

    // 计算归还的块位置，怎么拿走的就怎么归还
    int i = (s - m_start) / m_block_size;
    Mutex::Lock lock(m_mutex);
    m_blocks[i] = false; // 设置为未使用, 要上锁,设置未使用就用，里面数据不用清空，下次直接覆盖就行
    lock.unlock();

    m_ref_counts--;
}

bool Memory::hasBlock(char* s)
{
    return ((s >= m_start) && (s <= m_end));
}


} // namespace tinyrp
