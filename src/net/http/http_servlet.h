#ifndef SRC_NET_HTTP_HTTP_SERVLET_H
#define SRC_NET_HTTP_HTTP_SERVLET_H

// 作用就是处理中间过程，得到request处理

#include <memory>
#include "src/net/http/http_request.h"
#include "src/net/http/http_response.h"


namespace tinyrpc {

class HttpServlet : public std::enable_shared_from_this<HttpServlet> {

public:
    typedef std::shared_ptr<HttpServlet> ptr;

public:
// 这里不实现处理，只是在404继承中实现，因为处理中间过程放在了dispatcher中
    virtual void handle(HttpRequest* req, HttpResponse* res) = 0;  

    virtual std::string getServletName() = 0;

public:
    HttpServlet();
    
    virtual ~HttpServlet();

    void handleNotFound(HttpRequest* req, HttpResponse* res);  // 处理404

    void setHttpCode(HttpResponse* res, const int code); // 设置res处理结果code

    void setHttpContentType(HttpResponse* res, const std::string& content_type); // 设置返回的文本类型

    void setHttpBody(HttpResponse* res, const std::string& body);   // 设置返回的消息体

    void setCommParam(HttpRequest* req, HttpResponse* res);

};


class NotFoundHttpServlet : public HttpServlet {
public:
    NotFoundHttpServlet();

    ~NotFoundHttpServlet();

public:
    void handle(HttpRequest* req, HttpResponse* res);

    std::string getServletName();

};
    
} // namespace tinyrpc


#endif