// 两种协议的抽象数据类

#ifndef SRC_NET_ABSTRACT_DATA_H
#define SRC_NET_ABSTRACT_DATA_H

namespace tinyrpc{

class AbstractData{
public:
    AbstractData() = default;
    virtual ~AbstractData() {};

public:
    bool decode_succ {false};
    bool encode_succ {false};
};
    
} // namespace tingrpc



#endif