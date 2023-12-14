#include <cstring>
#include <unistd.h>
#include "src/net/tcp/tcp_buffer.h"
#include "src/comm/log.h"
#include "tcp_buffer.h"

tinyrpc::TcpBuffer::TcpBuffer(int size)
{
    m_buffer.resize(size);
}

tinyrpc::TcpBuffer::~TcpBuffer()
{
    clearBuffer();
}

int tinyrpc::TcpBuffer::readAble()
{
    return m_writeIndex - m_readIndex;
}

int tinyrpc::TcpBuffer::writeAble()
{
    return (int)m_buffer.size() - m_writeIndex;
}

int tinyrpc::TcpBuffer::readIndex() const
{
    return m_readIndex;
}

int tinyrpc::TcpBuffer::writeIndex() const
{
    return m_writeIndex;
}


void tinyrpc::TcpBuffer::resizeBuffer(int size)
{
    std::vector<char> tmp(size);

    int idx = std::min(size, readAble());
    // 可读部分移动到头部
    memcpy(&tmp[0], &m_buffer[m_readIndex], idx);

    m_buffer.swap(tmp);
    m_readIndex = 0;
    m_writeIndex = idx;
}

void tinyrpc::TcpBuffer::clearBuffer()
{
    m_buffer.clear();
    m_readIndex = 0;
    m_writeIndex = 0;
}
int tinyrpc::TcpBuffer::getSize()
{
    return (int)m_buffer.size();
}

std::vector<char> tinyrpc::TcpBuffer::getBufferVector()
{
    return m_buffer;
}

std::string tinyrpc::TcpBuffer::getBufferString()
{
    std::string tmp(readAble(), '0');
    memcpy(&tmp[0], &m_buffer[m_readIndex], readAble());
    return tmp;
}


// 写入到buffer
void tinyrpc::TcpBuffer::writeToBuffer(const char *buf, int size)
{
    // 容量不够，扩容
    if(size > writeAble())
    {
        // 扩容到1.5倍
        int newSize = (int)(1.5 * (writeAble() + size));
        resizeBuffer(newSize);
    }
    // 可写index后面加入写入的部分
    memcpy(&m_buffer[m_writeIndex], buf, size);
    m_writeIndex += size;
}

void tinyrpc::TcpBuffer::readFromBuffer(std::vector<char> &re, int size)
{
    if(readAble() <= 0)
    {
        DebugLog << "read buffer empty!";
        return;
    }

    memcpy(&re[0], &m_buffer[m_readIndex], size);
    m_readIndex += size;
    adjustBuffer();
}


void tinyrpc::TcpBuffer::recycleRead(int index)
{
    int j = m_readIndex + index;
    if (j > (int)m_buffer.size()) 
    {
        ErrorLog << "recycleRead error";
        return;
    }
    m_readIndex = j;
    adjustBuffer();
}

void tinyrpc::TcpBuffer::recucleWrite(int index)
{
    int j = m_writeIndex + index;
    if (j > (int)m_buffer.size()) 
    {
        ErrorLog << "recycleRead error";
        return;
    }
    m_writeIndex = j;
    adjustBuffer();
}

// 把可读字符移动到readINdex之前的位置，
void tinyrpc::TcpBuffer::adjustBuffer()
{
    // 用超过1/3的容量，就往前把空白的地方填充

    if(m_readIndex > static_cast<int>(m_buffer.size() / 3))
    {
        std::vector<char> tmp(m_buffer.size());

        int count = readAble();
        // 移动到前面
        memcpy(&tmp[0], &m_buffer[m_readIndex], count);

        m_buffer.swap(tmp);
        m_readIndex = 0;
        m_writeIndex = count;
    }
}
