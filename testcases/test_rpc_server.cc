#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <google/protobuf/service.h>
#include "rocket/common/log.h"
#include "rocket/net/tcp/tcp_server.h"
#include "rocket/net/tcp/tcp_client.h"
#include "rocket/net/coder/string_coder.h"
#include "rocket/net/coder/abstract_protocol.h"
#include "rocket/net/coder/tinypb_protocol.cc"
#include "rocket/net/coder/tinypb_coder.h"
#include "rocket/common/log.h"
#include "rocket/net/tcp/net_addr.h"
#include "rocket/net/tcp/tcp_server.h"
#include "rocket/net/rpc/rpc_dispatcher.h"

#include "order.pb.h"

class OrderIml : public Order
{
public:
    void makeOrder(google::protobuf::RpcController *controller,
                   const ::makeOrderRequest *request,
                   ::makeOrderResponse *response,
                   ::google::protobuf::Closure *done)
    {
        APPDEBUGLOG("start sleep 5s");
        sleep(5);
        APPDEBUGLOG("end sleep 5s");
        if (request->price() < 10)
        {
            response->set_ret_code(-1);
            response->set_res_info("short balance");
            return;
        }
        response->set_order_id("2024719");
    }
    ~OrderIml() {}
};

int main(int argc, char* argv[])
{
    if(argc!=2)
    {
        printf("Start test_rpc_server error,argc not 2\n");
        printf("Start like this\n");
        printf("./test_rpc_server 12345\n");
        return 0;
    }
    rocket::Logger::InitGlobalLogger(1);

    std::shared_ptr<OrderIml> service = std::make_shared<OrderIml>();
    rocket::RpcDispatcher::GetPrcDispatcher()->registerService(service);

    rocket::IPNetAddr::s_ptr addr = std::make_shared<rocket::IPNetAddr>("127.0.0.1", atoi(argv[1]));

    APPDEBUGLOG("create addr %s", addr->toString().c_str());

    rocket::TcpServer tcp_server(addr);

    tcp_server.start();

    return 0;
}
