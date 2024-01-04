#ifndef SRC_NET_HTTP_HTTP_REQUEST_H
#define SRC_NET_HTTP_HTTP_REQUEST_H

#include <string>
#include <memory>
#include <map>

#include "src/net/abstract_data.h"
#include "src/net/http/http_define.h"

namespace tinyrpc {
// 基本的请求内容
class HttpRequest : public AbstractData {

public:
    typedef std::shared_ptr<HttpRequest> ptr;

public:
    HttpMethod m_request_method; 
    std::string m_request_path;
    std::string m_request_version;

    std::string m_request_query;
    HttpRequestHeader m_requeset_header;
    std::string m_request_body;

    std::map<std::string, std::string> m_query_maps;
};
    
} // namespace tinyrpc


#endif