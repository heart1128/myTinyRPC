#include <algorithm>
#include <sstream>

#include "http_codec.h"
#include "src/comm/log.h"
#include "src/comm/string_util.h"
#include "src/net/abstract_data.h"
#include "src/net/abstract_codec.h"
#include "src/net/http/http_request.h"
#include "src/net/http/http_response.h"

namespace tinyrpc {


HttpCodeC::HttpCodeC()
{
}

HttpCodeC::~HttpCodeC()
{
}

// 编码到response
void HttpCodeC::encode(TcpBuffer *buf, AbstractData *data)
{
    // 这里buf还是读写buf，但是data作为虚基类，指向的是response的类

    DebugLog << "http encode start";
    HttpResponse* response = dynamic_cast<HttpResponse*>(data);
    response->encode_succ = false;

    // 组成就是response的整体格式
    std::stringstream ss;
    ss << response->m_response_version << " " << response->m_response_code << " "
        << response->m_response_info << "\r\n" << response->m_response_header.toHttpString() 
        << "\r\n" << response->m_response_body;
    
    std::string http_res = ss.str();
    DebugLog << "endcode http response is:  " << http_res;

    // 写入buf中等待发送
    buf->writeToBuffer(http_res.c_str(), http_res.length());

    DebugLog << "succ encode and write to buffer, writeindex=" << buf->writeIndex();
    response->encode_succ = true; // 编码成功
    DebugLog << "test encode end";

}

// 从buf中读取，然后写入到request中请求
void HttpCodeC::decode(TcpBuffer *buf, AbstractData *data)
{
    DebugLog << "http decode start";
    std::string strs = "";

    if (!buf || !data) 
    {
        ErrorLog << "decode error! buf or data nullptr";
        return;
    }

    // httprequest作为载体
    HttpRequest* request = dynamic_cast<HttpRequest*>(data);
    if(!request)
    {
        ErrorLog << "not http request type";
        return;
    }

    strs = buf->getBufferString();

    bool is_parse_request_line = false;
    bool is_parse_request_header = false;
    bool is_parse_request_content = false;
    // 分三步，状态机，每步出问题下一步就不行。

    int read_size = 0;
    std::string tmp(strs);
    DebugLog << "pending to parse str:" << tmp << ", total size =" << tmp.size();
    int len = tmp.length();

    while(true)
    {
        // 解析请求行
        // 方法 空格 URL 空格 版本 空格 \r\n
        if(!is_parse_request_line)
        {
            size_t i = tmp.find(g_CRLF); // 先找行结尾,find找到的是第一个\r\n的开头
            if(i == tmp.npos)
            {
                DebugLog << "not found CRLF in buffer while parse request line";
                return;
            }
            // 判断请整个buf除了请求行是否还有其他数据
            if(i == tmp.length() - 2)
            {
                DebugLog << "need to read more data while parse requset line";
                break;
            }
             // 开始解析行
            is_parse_request_line = parseHttpRequestLine(request, tmp.substr(0, i));
            if(!is_parse_request_line)
            {
                return; // 解析行失败
            }

            tmp = tmp.substr(i + 2, len - 2 - i); // 去掉请求行
            len = tmp.length();
            read_size = read_size + i + 2;
        }

        // 解析请求头
        // key:value\r\n
        if(!is_parse_request_header)
        {
            size_t j = tmp.find(g_CRLF_DOUBLE); // 找到空行
            if(j == tmp.npos)
            {
                DebugLog << "not found CRLF line in buffer while parst request header";
                return;
            }
            
            is_parse_request_header = parseHttpRequestHeader(request, tmp.substr(0, j));
            if(!is_parse_request_header)
            {
                return;
            }

            tmp = tmp.substr(j + 4, len - j - 4);
            len = tmp.length();
            read_size = read_size + j + 4;
        }

        // 解析请求体
        // 格式为 key=val&key=val&..... 
        if(!is_parse_request_content)
        {
            int content_len = std::atoi(request->m_requeset_header.m_maps["Content-Length"].c_str());
            if((int)strs.length() - read_size < content_len)
            {
                DebugLog << "need to read more data while parst request content";
                return;
            }

            // 判断请求方式
            if(request->m_request_method == POST && content_len != 0)
            {

            }
        }
    }

}

ProtocalType HttpCodeC::getProtocalType()
{
    return Http_Protocal;
}

bool HttpCodeC::parseHttpRequestLine(HttpRequest *request, const std::string &tmp)
{
    DebugLog << "parse request line start";
    // 取方法
    size_t i = tmp.find(" ");
    if(i == tmp.npos)
    {
        ErrorLog << "parse method error";
        return false;
    }
    std::string method = tmp.substr(0, i);
    std::transform(method.begin(), method.end(), method.begin(), toupper);
    if(method == "GET")
    {
        request->m_request_method = HttpMethod::GET;
    }
    else if(method == "POST")
    {
        request->m_request_method = HttpMethod::POST;
    }
    else
    {
        ErrorLog << "Unkonw method = " << method;
        return false;
    }
    DebugLog << "request method = " << method;
    i += 1;

    // 取URL
    size_t j = tmp.find(" ", i);
    if(j == tmp.npos)
    {
        ErrorLog << "parse URL error";
        return false;
    }
    std::string url = tmp.substr(i, j - i - 1);
    if(!urlIsCorrect(request, url))
    {
        ErrorLog << "URL build error" << url;
        return false;
    }
    i = j + 1;

    // 取http版本, 最后一点字段
    std::string version = tmp.substr(i , tmp.length() - i - 1);
    std::transform(version.begin(), version.end(), version.begin(), toupper);
    if(version != "HTTP/1.1" && version != "HTTP/1.0")
    {
        ErrorLog << "parse http request line error, not support http version : " << version; 
        return false;
    }
    request->m_request_version = version;                                                                                                                                
    DebugLog << "request http version = " << request->m_request_version;
    DebugLog << "parst request lien end";

    return true;

}

bool HttpCodeC::parseHttpRequestHeader(HttpRequest *request, const std::string &str)
{
    // 解析头就是键值对
    // str.len < 4 就表示连最后的\r\n\r\n都没有
    if(str.empty() || str.length() < 4 || str == "\r\n\r\n")
    {
        return false;
    }

    std::string tmp = str;
    // 分割键值对
    StringUtil::splitStrToMap(tmp, "\r\n", ":", request->m_requeset_header.m_maps);
    return true;
}

bool HttpCodeC::parseHttpRequestContent(HttpRequest *request, const std::string &str)
{
    if(str.empty())
    {
        return false;
    }
    request->m_request_body = str;
    return true;
}

bool HttpCodeC::urlIsCorrect(HttpRequest* request, std::string &url)
{
    size_t i = url.find("://");
    if(i != url.npos && i + 3 >= url.length()) 
    {
        ErrorLog << "parse http request request line error, bad url:" << url;
        return false;
    }    

    int len = 0;
    if(i == url.npos) // 没有整个url，不带https://，只有文件路径
    {
        DebugLog << "url only have path, url is" << url;
    }
    else
    {
        url = url.substr(i + 3, url.length() - i - 4); // 去除前面部分https://
        DebugLog << "delete http prefix, url = " << url;
        i = url.find_last_of("/"); // root or host
        len = url.length();
        if (i == url.npos || i == url.length() - 1) 
        {
            DebugLog << "http request root path, and query is empty";
            return true;
        }

        url = url.substr(i + 1, len - i - 1); // 真实的请求文件路径
    }

    len = url.length();
    i = url.find_first_of('?'); // 这个之后是请求的参数，key:val
    if (i == url.npos) // 没有后面的参数，就直接设置url
    {
        request->m_request_path = url;
        DebugLog << "http request path:" << request->m_request_path << "and query is empty";
        return true;
    }

    // 有后面的参数
    request->m_request_path = url.substr(0, i);
    request->m_request_query = url.substr(i + 1, len - i - 1);
    DebugLog << "http request path:" << request->m_request_path << ", and query:" << request->m_request_query;

    // 分割参数成map键值对
    StringUtil::splitStrToMap(request->m_request_query, "&", "=", request->m_query_maps);
    return true;
}

} // namespace tinyrpc
