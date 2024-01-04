#ifndef SRC_NET_HTTP_HTTP_DISPATCHER_H
#define SRC_NET_HTTP_HTTP_DISPATCHER_H

#include <map>
#include <memory>
#include "src/net/abstract_dispatcher.h"
#include "src/net/http/http_servlet.h"


namespace tinyrpc {
    
class HttpDispatcher : public AbstractDispatcher{

public:
    HttpDispatcher() = default;
    ~HttpDispatcher() = default;

public:
    void dispatch(AbstractData* data, TcpConnection* conn);

    void registerServlet(const std::string& path, HttpServlet::ptr servlet);

public:
    std::map<std::string, HttpServlet::ptr> m_servlets;

};

} // namespace tinyrpc



#endif