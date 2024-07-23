#ifndef ROCKET_NET_RPC_RPC_CONTROLLER_H
#define ROCKET_NET_RPC_RPC_CONTROLLER_H

#include <google/protobuf/service.h>
#include <google/protobuf/stubs/callback.h>
#include <string>
#include "rocket/net/tcp/net_addr.h"
#include <google/protobuf/stubs/callback.h>
namespace rocket
{
    
    class RpcController : public google::protobuf::RpcController
    {
    public:
        RpcController(){};
        ~RpcController(){};

        void Reset();

        bool Failed() const;

        std::string ErrorText() const;

        void StartCancel();

        void SetFailed(const std::string &reason);

        bool IsCanceled() const;

        void NotifyOnCancel(google::protobuf::Closure *callback);

        void SetError(int32_t errcode,const std::string error_info);

        int32_t GetErrorCode();

        std::string GetErrorInfo();

        void SetMsgId(std::string req_id);

        std::string GetMsgId();

        void SetLocalAddr(NetAddr::s_ptr addr);

        NetAddr::s_ptr GetLocalAddr();

        void SetPeerAddr(NetAddr::s_ptr addr);

        NetAddr::s_ptr GetPeerAddr();

        void SetTimeout(int timeout);

        int GetTimeout();

    private:
        int32_t m_error_code{0};
        std::string m_error_info;
        std::string m_msg_id;

        bool m_is_failed{false};
        bool m_is_cancle{false};

        NetAddr::s_ptr m_local_addr;
        NetAddr::s_ptr m_peer_addr;

        int m_timeout{1000};
    };
}

#endif