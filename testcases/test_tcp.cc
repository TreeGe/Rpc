#include <memory.h>
#include "rocket/common/log.h"
#include "rocket/net/tcp/net_addr.h"
#include "rocket/net/tcp/tcp_server.h"

void test_tcp_server()
{
    rocket::IPNetAddr::s_ptr addr = std::make_shared<rocket::IPNetAddr>("127.0.0.1",12345);

    DEBUGLOG("create addr %s",addr->toString().c_str());

    rocket::TcpServer tcp_server(addr);

    tcp_server.start();

}


int main()
{
    rocket::Logger::InitGlobalLogger();

    // rocket::IPNetAddr addr("127.0.0.1",12345);
    // DEBUGLOG("create addr %s ",addr.toString().c_str());

    test_tcp_server();

    return 0;
}