#include <google/protobuf/service.h>
#include <memory>
#include "src/net/http/http_dispatcher.h"
#include "src/net/http/http_request.h"
#include "src/net/http/http_servlet.h"
#include "src/comm/log.h"
#include "src/comm/msg_req.h"


namespace tinyrpc {

// 在conn的exture执行
void HttpDispatcher::dispatch(AbstractData *data, TcpConnection *conn)
{
    HttpRequest* request = dynamic_cast<HttpRequest*>(data);
    HttpResponse response;

    // 运行编号都要
    Coroutine::getCurrentCoroutine()->getRunTime()->m_msg_no = MsgReqUtil::genMsgNumber();
    setCurrentRunTime(Coroutine::getCurrentCoroutine()->getRunTime());

    InfoLog << "begin to dispatch client http request, msgno=" << Coroutine::getCurrentCoroutine()->getRunTime()->m_msg_no;

    // 分发实际上就是处理编解码，填写servlet信息
    std::string url_path = request->m_request_path;

    if(!url_path.empty())
    {
        // 找到之前注册的servlet
        auto it = m_servlets.find(url_path);

        if (it == m_servlets.end()) // 找不到注册的服务就是404
        {
            ErrorLog << "404, url path{ " << url_path << "}, msgno=" << Coroutine::getCurrentCoroutine()->getRunTime()->m_msg_no;
            // 返回404网页
            NotFoundHttpServlet servlet;
            Coroutine::getCurrentCoroutine()->getRunTime()->m_interface_name = servlet.getServletName();
            servlet.setCommParam(request, &response);
            servlet.handle(request, &response);
        }
        else
        {
            Coroutine::getCurrentCoroutine()->getRunTime()->m_interface_name = it->second->getServletName();
            it->second->setCommParam(request, &response);
            it->second->handle(request, &response); // 制作response
        }

        conn->getCodec()->encode(conn->getOutBuffer(), &response); // 编码是之前指定的http编码，这里就是制作response
        InfoLog << "end dispatch client http request, msgno=" << Coroutine::getCurrentCoroutine()->getRunTime()->m_msg_no;
    }

}

void HttpDispatcher::registerServlet(const std::string &path, HttpServlet::ptr servlet)
{
    // 注册到一个map中
    auto it = m_servlets.find(path);
    // 没有加入过就直接加入
    if(it == m_servlets.end())
    {
        DebugLog << "register servlet success to path {" << path << "}";
        m_servlets[path] = servlet;
    }
    else
    {
        ErrorLog << "failed to register, beacuse path {" << path << "} has already register sertlet";
    }
}
}