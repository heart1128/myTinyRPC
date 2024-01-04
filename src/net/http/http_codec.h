#ifndef SRC_NET_HTTP_HTTP_CODEC_H
#define SRC_NET_HTTP_HTTP_CODEC_H

#include <map>
#include <string>
#include "src/net/abstract_data.h"
#include "src/net/abstract_codec.h"
#include "src/net/http/http_request.h"

// 编解码。解码解析request，编码组成response

namespace tinyrpc {

class HttpCodeC : public AbstractCodeC {

public:
    HttpCodeC();
    ~HttpCodeC();

public:
    void encode(TcpBuffer* buf, AbstractData* data);

    void decode(TcpBuffer* buf, AbstractData* data);

    ProtocalType getProtocalType();

private:
    bool parseHttpRequestLine(HttpRequest* request, const std::string& str);
    bool parseHttpRequestHeader(HttpRequest* request, const std::string& str);
    bool parseHttpRequestContent(HttpRequest* request, const std::string& str);

    bool urlIsCorrect(HttpRequest* request, std::string& url);

};


    
} // namespace tinyrpc


#endif