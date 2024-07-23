#include <google/protobuf/service.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include "rocket/net/coder/tinypb_protocol.h"
#include "rocket/net/rpc/rpc_controller.h"
#include "rocket/net/rpc/rpc_channel.h"
#include "rocket/common/log.h"
#include "rocket/common/msg_id_util.h"
#include "rocket/net/tcp/tcp_client.h"
#include "rocket/common/error_code.h"
#include "rocket/net/timer_event.h"
#include "rocket/common/util.h"

namespace rocket
{
    RpcChannel::RpcChannel(NetAddr::s_ptr peer_addr) : m_peer_addr(peer_addr)
    {
        m_client = std::make_shared<TcpClient>(m_peer_addr);
    }
    RpcChannel::~RpcChannel()
    {
        DEBUGLOG("~RpcChannel");
    }

    void rocket::RpcChannel::Init(controller_s_ptr controller, message_s_ptr req, message_s_ptr rsp, closure_s_ptr done)
    {
        if(m_is_init)
        {
            return;
        }
        m_controller=controller;
        m_request=req;
        m_response=rsp;
        m_closure=done;
        m_is_init=true;
    }

    void rocket::RpcChannel::CallMethod(const google::protobuf::MethodDescriptor *method, google::protobuf::RpcController *controller, const google::protobuf::Message *request, google::protobuf::Message *response, google::protobuf::Closure *done)
    {
        //先定义一个发送的信息
        std::shared_ptr<rocket::TinypbProtocol> req_protocol = std::make_shared<rocket::TinypbProtocol>();

        rocket::RpcController *my_controller = dynamic_cast<rocket::RpcController*>(controller);
        if(my_controller==NULL)
        {
            // ERRORLOG("failed callmethod,RpcController convert error");
            printf("failed callmethod,RpcController convert error\n");
            return;
        }


        if(my_controller->GetMsgId().empty())
        {
            req_protocol->m_msg_id = MsgIdUtil::GenMsgId();
            my_controller->SetMsgId(req_protocol->m_msg_id);
        }
        else
        {
            req_protocol->m_msg_id = my_controller->GetMsgId();
        }
        req_protocol->m_method_name = method->full_name();
        INFOLOG("%s | call method name [%s]",req_protocol->m_msg_id.c_str(),req_protocol->m_method_name.c_str());

        if (!m_is_init)
        {
            std::string error_info = "RpcChannel not init";
            my_controller->SetError(ERROR_RPC_CHANNEL_INIT,error_info);
            ERRORLOG("%s | %s ,RpcChannel not init",req_protocol->m_msg_id.c_str(),error_info.c_str());
            return ;
        }

        //requset 的序列化
        if(!request->SerializeToString(&(req_protocol->m_pb_data)))
        {
            std::string error_info = "failed to serizlize";
            my_controller->SetError(ERROR_FAILED_SERIALIZE,error_info);
            ERRORLOG("%s | %s ,origin requset [%s]",req_protocol->m_msg_id.c_str(),error_info.c_str(),request->ShortDebugString().c_str());
            return ;
        }

        s_ptr channel = shared_from_this();

        m_time_event = std::make_shared<TimeEvent>(my_controller->GetTimeout(),false,[my_controller, channel]() mutable
        {
            INFOLOG("come in callback!");
            my_controller->StartCancel();
            my_controller->SetError(ERROR_RPC_CALL_TIMEOUT,"rpc call timeout "+std::to_string(my_controller->GetTimeout()));

            if(channel->getClosure())
            {
                channel->getClosure()->Run();
            }
            channel.reset();

        });

        m_client->addTimerEvent(m_time_event);

        channel->getTcpClient()->connect([req_protocol,channel]() mutable
        {
            RpcController* my_controller = dynamic_cast<RpcController*>(channel->getController());
            if(channel->getTcpClient()->getConnectErrorCode()!=0)
            {
                ERRORLOG("%s | connect error , error code[%d], error info[%s],peer addr[%s]",req_protocol->m_msg_id.c_str(),
                    my_controller->GetErrorCode(),my_controller->GetErrorInfo().c_str(),channel->getTcpClient()->getPeerAddr()->toString().c_str());
                my_controller->SetError(channel->getTcpClient()->getConnectErrorCode(),channel->getTcpClient()->getConnectErrorInfo());
                return;
            }
            channel->getTcpClient()->writeMessage(req_protocol,[req_protocol,channel,my_controller](AbstractProtocol::s_ptr msg1) mutable
            {
                INFOLOG("%s | ,send rpc request success. call method name [%s],peer addr[%s] ,local addr[%s]",
                req_protocol->m_msg_id.c_str(),req_protocol->m_method_name.c_str(),
                channel->getTcpClient()->getPeerAddr()->toString().c_str(),channel->getTcpClient()->getLocalAddr()->toString().c_str());

                channel->getTcpClient()->readMessage(req_protocol->m_msg_id,[channel,my_controller](AbstractProtocol::s_ptr msg) mutable
                {
                    std::shared_ptr<rocket::TinypbProtocol> rsp_message = std::dynamic_pointer_cast<rocket::TinypbProtocol>(msg);
                    INFOLOG("%s | success get rpc response,call method name [%s],peer addr[%s] ,local addr[%s]",
                    rsp_message->m_msg_id.c_str(),rsp_message->m_method_name.c_str(),
                        channel->getTcpClient()->getPeerAddr()->toString().c_str(),channel->getTcpClient()->getLocalAddr()->toString().c_str());

                    INFOLOG("readMessage time = %ld",getNowMs());
                    // 取消定时任务
                    channel->getTimeEvent()->setCancle(true);

                    if(!(channel->getResponse()->ParseFromString(rsp_message->m_pb_data)))
                    {
                        ERRORLOG("%s | serialize error",rsp_message->m_msg_id.c_str());
                        my_controller->SetError(ERROR_FAILED_SERIALIZE,"serialize error");
                        return;
                    }

                    if(rsp_message->m_err_code!=0)
                    {
                        ERRORLOG("%s | call rpc method[%s] failed, error code [%d], error info[%s]",rsp_message->m_msg_id.c_str(),
                        rsp_message->m_method_name.c_str(),rsp_message->m_err_code,rsp_message->m_err_info.c_str());

                        my_controller->SetError(rsp_message->m_err_code,rsp_message->m_err_info);
                        return;

                    }

                    INFOLOG("%s | call rpc success , call rpc method[%s], peer addr[%s] ,local addr[%s]", 
                        rsp_message->m_msg_id.c_str(),rsp_message->m_method_name.c_str(),
                        channel->getTcpClient()->getPeerAddr()->toString().c_str(),channel->getTcpClient()->getLocalAddr()->toString().c_str())

                    
                    if(!my_controller->IsCanceled() && channel->getClosure())
                    {
                        channel->getClosure()->Run();
                    }

                    channel.reset();
                });
            });
        });
    }
    google::protobuf::RpcController * RpcChannel::getController()
    {
        return m_controller.get();
    }
    google::protobuf::Message * RpcChannel::getRequest()
    {
        return m_request.get();
    }
    google::protobuf::Message * RpcChannel::getResponse()
    {
        return m_response.get();
    }
    google::protobuf::Closure * RpcChannel::getClosure()
    {
        return m_closure.get();
    }
    TcpClient* RpcChannel::getTcpClient()
    {
        return m_client.get();
    }

    TimeEvent::s_ptr RpcChannel::getTimeEvent()
    {
        return m_time_event;
    }
}