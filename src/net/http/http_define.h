// 记录一些错误码、请求头字段
#ifndef SRC_HTTP_HTTP_DEFINE_H
#define SRC_HTTP_HTTP_DEFINE_H


#include <string>
#include <map>

namespace tinyrpc {

extern std::string g_CRLF; // 头部和消息体中间的换行
extern std::string g_CRLF_DOUBLE;

extern std::string content_type_text; // 内容文本类型 json,txt ...
extern const char* default_html_template; // 最简单的html

// 请求方法
enum HttpMethod {
    GET = 1,
    POST = 2
};

// 常用状态代码
enum HttpCode {
    HTTP_OK = 200,
    HTTP_BADREQUEST = 400,
    HTTP_FORBIDDEN = 403,
    HTTP_NOTFOUND = 404,
    HTTP_INTERNALSERVERERROR = 500  // 服务器内部错误
};

const char* httpCodeToString(const int code);

class HttpHeaderComm
{

public:
    HttpHeaderComm() = default;
    virtual ~HttpHeaderComm() = default;  // 被人继承

public:
    int getHeaderTotalLength(); 

    std::string getValue(const std::string& key);

    void setKeyValue(const std::string& key, const std::string& value);

    std::string toHttpString();

public:
    std::map<std::string, std::string> m_maps; // 请求/响应头的字段
};


// 单纯区分一下
class HttpRequestHeader : public HttpHeaderComm {

};

class HttpResponseHeader : public HttpHeaderComm {

};



    
} // namespace tinyrp 



#endif