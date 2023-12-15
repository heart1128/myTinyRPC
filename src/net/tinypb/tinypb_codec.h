#ifndef SRC_NET_TINYPB_TINYPB_CODEC_H
#define SRC_NET_TINYPB_TINYPB_CODEC_H

// 实现protobuf数据流的编解码

#include <stdint.h>
#include "src/net/abstract_codec.h"
#include "src/net/abstract_data.h"
#include "src/net/tinypb/tinypb_data.h"

namespace tinyrpc{

class TinyPbCodeC : public AbstractCodeC{
public:

    TinyPbCodeC();
    ~TinyPbCodeC();

    // 重载函数三个
    void encode(TcpBuffer* buf, AbstractData* data) override;

    void decode(TcpBuffer* buf, AbstractData* data) override;

    // 再继承定义
    virtual ProtocalType getProtocalType();
    
    const char* encodePbData(TinyPbStruct* data, int& len);

};
   
} // namespace tinyrpc


#endif