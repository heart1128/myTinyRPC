#ifndef SRC_NET_CODEC_H
#define SRC_NET_CODEC_H

// 编解码抽象类。需要继承实现encode decode

#include <string>
#include <memory>
#include "tcp/tcp_buffer.h"
#include "abstract_data.h"

namespace tinyrpc {

// 设计了两种协议
enum ProtocalType{
    TinyPb_Protocal = 1,
    Http_Protocal = 2
};

class AbstractCodeC{
public:
    typedef std::shared_ptr<AbstractCodeC> ptr;

public:
    AbstractCodeC(){}
    // 继承使用虚析构，防止使用父类指针指向子类实例，析构是只是析构父类，子类不析构
    virtual ~AbstractCodeC(){}

    virtual void encode(TcpBuffer* buf, AbstractData* data) = 0;

    virtual void decode(TcpBuffer* buf, AbstractData* data) = 0;

    virtual ProtocalType getProtocalType() = 0;
};
    
} // namespace tinyrpc


#endif