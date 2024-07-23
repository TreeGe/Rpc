#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "rocket/common/log.h"
#include "rocket/net/tcp/tcp_server.h"
#include "rocket/net/tcp/tcp_client.h"
#include "rocket/net/coder/string_coder.h"
#include "rocket/net/coder/abstract_protocol.h"
#include "rocket/net/coder/tinypb_protocol.cc"
#include "rocket/net/coder/tinypb_coder.h"
#include "order.pb.h"
#include "rocket/net/rpc/rpc_channel.h"
#include "rocket/net/rpc/rpc_controller.h"
#include "rocket/net/rpc/rpc_closure.h"


void test_tcp_client()
{
    rocket::IPNetAddr::s_ptr addr = std::make_shared<rocket::IPNetAddr>("127.0.0.1", 12345);
    rocket::TcpClient client(addr);

    client.connect([addr,&client]()
    {
        DEBUGLOG("connect to [%s] success",addr->toString().c_str());
        // printf("connect to [%s] success\n",addr->toString().c_str());
        std::shared_ptr<rocket::TinypbProtocol> message = std::make_shared<rocket::TinypbProtocol>();
        message->m_msg_id = "99999999999898";
        message->m_pb_data = "test pb data";

        makeOrderRequest request;
        request.set_price(100);
        request.set_goods("apple");
        
        if(!request.SerializeToString(&(message->m_pb_data)))
        {
            printf("serilize error\n");
            return;
        }

        message->m_method_name = "Order.makeOrder";

        client.writeMessage(message,[request](rocket::AbstractProtocol::s_ptr msg_ptr)
        {
            printf("send messages success,request[%s] \n",request.ShortDebugString().c_str());
        });

        client.readMessage("99999999999898",[](rocket::AbstractProtocol::s_ptr msg_ptr)
        {
            std::shared_ptr<rocket::TinypbProtocol> message = std::dynamic_pointer_cast<rocket::TinypbProtocol>(msg_ptr);
            printf("req_id[%s], get response %s",message->m_msg_id.c_str(),message->m_pb_data.c_str());
            makeOrderResponse response;

            if(!response.ParseFromString(message->m_pb_data))
            {
                printf("deserialize error\n");
                return;
            }
            printf("get response success ,respones[%s]",response.ShortDebugString().c_str());
        });

    });
}


void test_rpc_channel()
{
    NEWRPCCHANNEL("127.0.0.1:12345",channel);
    NEWMESSAGE(makeOrderRequest, request);
    NEWMESSAGE(makeOrderResponse, response);

    request->set_price(100);
    request->set_goods("apple");

    NEWRPCCONTROLLER(controller);
    controller->SetMsgId("77777776");
    controller->SetTimeout(10000);

    std::shared_ptr<rocket::RpcClosure> closure = std::make_shared<rocket::RpcClosure>([request, response, channel, controller]() mutable
                                                                                       {
        if(controller->GetErrorCode()==0)
        {
            INFOLOG("call rpc success , request[%s] , respone[%s]\n",request->ShortDebugString().c_str(),response->ShortDebugString().c_str());
            //业务逻辑
            // if(response->order_id=="xxx")
            // {
            //     //xxx
            // }
        }
        else
        {
            ERRORLOG("call rpc failed , request[%s],error code[%d], error info[%s] \n",
            request->ShortDebugString().c_str(),
            controller->GetErrorCode(),controller->GetErrorInfo().c_str());
        }
        INFOLOG("now exit eventloop");
        channel->getTcpClient()->stop();
        channel.reset(); });

        CALLRPC("127.0.0.1:12345",makeOrder,controller,request,response,closure);
}
int main()
{
    rocket::Logger::InitGlobalLogger(2);

    // test_connect();

    // test_tcp_client();

    test_rpc_channel();

    INFOLOG("test rpc channel end");

    return 0;
}