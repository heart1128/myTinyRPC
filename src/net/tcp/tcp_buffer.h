#ifndef SRC_NET_TCP_TCP_BUFFER_H
#define SRC_NET_TCP_TCP_BUFFER_H

#include <vector>
#include <memory>

/*

    vector作为一个buffer
    read_index作为读位置
    write_index作为写位置

*/

namespace tinyrpc{

class TcpBuffer{
public:
    typedef std::shared_ptr<TcpBuffer> ptr;

public:
    TcpBuffer(int size);
    ~TcpBuffer();

public:
    int readAble();  // 可读数
    int writeAble(); // 可写数

    int readIndex() const;
    int writeIndex() const;

    void writeToBuffer(const char* buf, int size);
    void readFromBuffer(std::vector<char>& re, int size);

    void resizeBuffer(int size);

    void clearBuffer();
    int getSize();

    std::vector<char> getBufferVector();
    std::string getBufferString();

    void recycleRead(int index);
    void recucleWrite(int index);

    void adjustBuffer();


public:
    std::vector<char> m_buffer;
private:
    int m_readIndex {0};
    int m_writeIndex {0};
};

    
} // namespace tinyrpc



#endif