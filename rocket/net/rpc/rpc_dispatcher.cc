#include <google/protobuf/service.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include "rocket/net/rpc/rpc_dispatcher.h"
#include "rocket/net/coder/tinypb_protocol.h"
#include "rocket/common/error_code.h"
#include "rocket/net/rpc/rpc_controller.h"
#include "rocket/net/tcp/net_addr.h"
#include "rocket/net/tcp/tcp_connection.h"
#include "rocket/common/runtime.h"

namespace rocket
{
    static RpcDispatcher* g_rpc_dispatcher = NULL;
    RpcDispatcher* RpcDispatcher::GetPrcDispatcher()
    {
        if(g_rpc_dispatcher!=NULL)
        {
            return g_rpc_dispatcher;
        }
        g_rpc_dispatcher = new RpcDispatcher;
        return g_rpc_dispatcher;
    }
    void rocket::RpcDispatcher::dispather(AbstractProtocol::s_ptr request, AbstractProtocol::s_ptr respone,TcpConnection* connection)
    {
        // 1. 从buufer读取数据，然后 decode 得到请求的 TinyPBProtobol 对象。然后从请求的 TinyPBProtobol 得到 method_name, 从 OrderService 对象里根据 service.method_name 找到方法 func
        // 2. 找到对应的 requeset type 以及 response type
        // 3. 将请求体 TinyPBProtobol 里面的 pb_date 反序列化为 requeset type 的一个对象, 声明一个空的 response type 对象
        // 4. func(request, response)
        // 5. 将 reponse 对象序列为 pb_data。 再塞入到 TinyPBProtobol 结构体中。做 encode 然后塞入到buffer里面，就会发送回包了
        // 拿到协议对象 然后去method_name
        std::shared_ptr<TinypbProtocol> req_protocol = std::dynamic_pointer_cast<TinypbProtocol>(request);
        std::shared_ptr<TinypbProtocol> rsp_protocol = std::dynamic_pointer_cast<TinypbProtocol>(respone);

        std::string method_full_name = req_protocol->m_method_name;
        std::string service_name;
        std::string method_name;

        rsp_protocol->m_msg_id = req_protocol->m_msg_id;
        rsp_protocol->m_method_name = req_protocol->m_method_name;

        if(!parseServiceFullName(method_full_name,service_name,method_name))
        {
            setTinypberror(rsp_protocol,ERROR_PARSE_SERVICE_NAME,"parse service name error");
            return;
        }
        auto it=m_service_map.find(service_name);
        if(it==m_service_map.end())
        {
            printf("serveice not found \n");
            setTinypberror(rsp_protocol,ERROR_SERIVIE_NOT_FOUND,"service not found");
            return;
        }
        service_s_ptr service = (*it).second;

        const google::protobuf::MethodDescriptor* method = service->GetDescriptor()->FindMethodByName(method_name);
        if(method==NULL)
        {
            printf("method name[%s] not found in service[%s]\n",method_name.c_str(),service_name.c_str());
            setTinypberror(rsp_protocol,ERROR_METHOD_NOT_FOUND,"method not found");
            return;
        }

        google::protobuf::Message* req_msg = service->GetRequestPrototype(method).New();  //获取request type

        //反序列化 将pb_data 反序列化为req_msg
        if(!req_msg->ParseFromString(req_protocol->m_pb_data))
        {
            printf("[%s] | deserilize error\n",req_protocol->m_msg_id.c_str());
            setTinypberror(rsp_protocol,ERROR_FAILED_DESERIALIZE,"deserialize falied");
            if(req_msg!=NULL)
            {
                delete req_msg;
                req_msg=NULL;
            }
            return;
        }
        printf("req_id[%s] |  get rpc request[%s]",req_protocol->m_msg_id.c_str(),req_msg->ShortDebugString().c_str());


        google::protobuf::Message* rsp_msg = service->GetResponsePrototype(method).New(); //获取response type


        RpcController rpcController;
        IPNetAddr::s_ptr local_addr = std::make_shared<IPNetAddr>("127.0.0.1",1234);
        rpcController.SetLocalAddr(connection->getLocalAddr());
        rpcController.SetPeerAddr(connection->getPeerAddr());
        rpcController.SetMsgId(req_protocol->m_msg_id);
        
        RunTime::GetRunTime()->m_msgid = req_protocol->m_msg_id;
        RunTime::GetRunTime()->m_method_name = method_name;
        service->CallMethod(method,&rpcController,req_msg,rsp_msg,NULL);
        
        if(!rsp_msg->SerializeToString(&(rsp_protocol->m_pb_data)))
        {
            printf("[%s] | serilize error, origin message[%s]\n",req_protocol->m_msg_id.c_str(),rsp_msg->ShortDebugString().c_str());
            setTinypberror(rsp_protocol,ERROR_FAILED_SERIALIZE,"serialize falied");
            if(req_msg!=NULL)
            {
                delete req_msg;
                req_msg=NULL;
            }
            if(rsp_msg!=NULL)
            {
                delete req_msg;
                rsp_msg=NULL;
            }
            return;
        }
        rsp_protocol->m_err_code = 0;       
        printf("%s | dispatch success , requset[%s], respone[%s]",req_protocol->m_msg_id.c_str(), req_msg->ShortDebugString().c_str(), rsp_msg->ShortDebugString().c_str());
        
        delete req_msg;
        req_msg = NULL;
        delete rsp_msg;
        rsp_msg = NULL;
    }

    void rocket::RpcDispatcher::registerService(service_s_ptr service)
    {
        // service = std::make_shared<google::protobuf::Service>();
        std::string service_name = service->GetDescriptor()->full_name();
        if(m_service_map.find(service_name)==m_service_map.end())
        {
            m_service_map.insert(std::make_pair(service_name,service));
            return ;
        }
        m_service_map[service_name] = service;
    }

    void rocket::RpcDispatcher::setTinypberror(std::shared_ptr<TinypbProtocol> msg, int32_t err_code, const std::string err_info)
    {
        msg->m_err_code = err_code;
        msg->m_err_info = err_info;
        msg->m_err_info_len = err_info.size();
    }

    bool rocket::RpcDispatcher::parseServiceFullName(const std::string &full_name, std::string &service_name, std::string &method_name)
    {
        if(full_name.empty())
        {
            printf("full name is empty\n");
            return false;
        }
        size_t i =full_name.find_first_of(".");
        if(i==full_name.npos)
        {
            printf("not found . in full name[%s]\n", full_name.c_str());
            return false;
        }
        service_name = full_name.substr(0,i);
        method_name = full_name.substr(i+1,full_name.length()-i-1);

        printf("parse service_name[%s] and method_name [%s] from full name [%s]\n",service_name.c_str(),method_name.c_str(),full_name.c_str());
        return true;
    }
}
