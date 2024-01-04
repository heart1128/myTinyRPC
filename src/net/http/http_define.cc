#include <string>
#include <sstream>
#include "src/net/http/http_define.h"

namespace tinyrpc {

std::string g_CRLF = "\r\n"; // 回车+换行 换到下一行，并且是开头
std::string g_CRLF_DOUBLE = "\r\n\r\n";

std::string content_type_text = "text/html;charset=utf-8";
const char* default_html_template = "<html><body><h1>%s</h1><p>%s</p></body></html>";



const char* httpCodeToString(const int code)
{
    switch (code)
    {
        case HTTP_OK:
            return "OK";
        
        case HTTP_BADREQUEST:
            return "bad request";
        
        case HTTP_FORBIDDEN:
            return "Forbidden";

        case HTTP_NOTFOUND:
            return "Not Found";

        case HTTP_INTERNALSERVERERROR:
        return "Internal Server Error";
        
        default:
            return "Unknown code";
    }
    
}


int HttpHeaderComm::getHeaderTotalLength()
{
    // 请求头中所有字符个数
    // key : val \r\n   所以key后面多加1，val后面多加2
    int len = 0;
    for (const auto& it : m_maps)
    {
        len += it.first.length() + 1 + it.second.length() + 2;
    }

    return len;
}

std::string HttpHeaderComm::getValue(const std::string &key)
{
    return m_maps[key.c_str()];
}

void HttpHeaderComm::setKeyValue(const std::string &key, const std::string &value)
{
    m_maps[key.c_str()] = value;
}

std::string HttpHeaderComm::toHttpString()
{
    std::stringstream ss;
    for (const auto& it : m_maps)
    {
        ss << it.first << ":" << it.second << "\r\n";
    }

    return ss.str();
}

} // namespace tinyrpc

