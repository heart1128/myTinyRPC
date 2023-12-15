#ifndef SRC_NET_TINYPB_TINYPB_RPC_CONRTOLLER_H
#define SRC_NET_TINYPB_TINYPB_RPC_CONRTOLLER_H

#include <google/protobuf/service.h>
#include <google/protobuf/stubs/callback.h>
#include <stdio.h>
#include <memory>
#include "../net_address.h"

// 需要实现几个必须的，分别为客户端和服务端使用的方法
// 然后可以自己添加更多的方法辅助

namespace tinyrpc {

class TinyPbRpcController : public google::protobuf::RpcController {
public:
    TinyPbRpcController() = default;

    ~TinyPbRpcController() = default;

public:
    typedef std::shared_ptr<TinyPbRpcController> ptr;
    // Client-side methods ---------------------------------------------
    // 这些调用只能从客户端进行。他们的结果在服务器端是未定义的.

    
    // 将RpcController重置为初始状态，以便可以在新的调用中重用它。RPC正在进行时不能被调用。
    void Reset() override;
    // 调用结束后，如果调用失败，则返回true。失败的原因取决于RPC实现，在调用完成之前，不能调用Failed，
    // 如果Failed返回true，则response消息的内容未定义。
    bool Failed() const override;


    // Server-side methods ---------------------------------------------
    // 如果Failed为true，则返回人类可读的错误描述。
    std::string ErrorText() const override;
    // 建议RPC系统调用者希望RPC调用被取消。RPC系统可能会立即取消它，可能会等待一段时间然后取消它，甚至可能根本不会取消该调用。  
    // 如果该调用被取消，则“done”回调将仍然被调用，并且RpcController将指示此时调用失败。
    void StartCancel() override;
    // 使得Failed在客户端返回true。
    void SetFailed(const std::string& reason) override;
    // 如果为true，则表示客户端取消了RPC，因此服务器可能放弃回复它。服务器仍然应该调用最终的“done”回调。
    bool IsCanceled() const override;
    // 要求在取消RPC时调用给定的回调。回调将始终被调用一次。如果RPC完成而未被取消，则完成后将调用回调。
    // 如果在调用NotifyOnCancel时RPC已被取消，则立即调用回调。NotifyOnCancel（）每个请求只能调用一次。
    void NotifyOnCancel(google::protobuf::Closure* callback) override;


    // common methods

    int ErrorCode() const;

    void SetErrorCode(const int error_code);

    const std::string& MsgSeq() const;

    void SetMsgReq(const std::string& msg_req);

    void SetError(const int err_code, const std::string& err_info);

    void SetPeerAddr(NetAddress::ptr addr);

    void SetLocalAddr(NetAddress::ptr addr);

    NetAddress::ptr PeerAddr();
    
    NetAddress::ptr LocalAddr();

    void SetTimeout(const int timeout);

    int Timeout() const;

    void SetMethodName(const std::string& name);

    std::string GetMethodName();

    void SetMethodFullName(const std::string& name);

    std::string GetMethodFullName();



private:
    int m_error_code {0};           // error_code, identify one specific error
    std::string m_error_info;       // error_info, details description of error
    std::string m_msg_req;          // msg_req, identify once rpc request and response
    bool m_is_failed {false}; 
    bool m_is_cancled {false};
    NetAddress::ptr m_peer_addr;
    NetAddress::ptr m_local_addr;

    int m_timeout {5000};           // max call rpc timeout
    std::string m_method_name;      // method name
    std::string m_full_name;        // full name, like server.method_name
};

}



#endif