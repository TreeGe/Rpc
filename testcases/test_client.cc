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

void test_connect()
{
    // 调用connect 连接 server
    // write 一个字符串
    // 等待 read 返回结果

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
    {
        ERRORLOG("invalid listenfd %d", fd);
        exit(0);
    }

    sockaddr_in server_addr;
    memset(&server_addr,0,sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(12345);
    inet_aton("127.0.0.1",&server_addr.sin_addr);

    int rt = connect(fd,reinterpret_cast<sockaddr*>(&server_addr),sizeof(server_addr));

    if(rt==0)
    {
        DEBUGLOG("connect success!\n");
    }

    std::string msg = "hello rocket!\n";

    rt = write(fd,msg.c_str(),msg.length());

    // DEBUGLOG("success write %d bytes ,[%s]",rt,msg.c_str());
    printf("success write %d bytes ,[%s]\n",rt,msg.c_str());

    char buf[100];
    rt = read(fd,buf,100);

    // INFOLOG("success read %d bytes , [%s]",rt,std::string(buf).c_str());
    printf("success read %d bytes , [%s]\n",rt,std::string(buf).c_str());
}


void test_tcp_client()
{
    rocket::IPNetAddr::s_ptr addr = std::make_shared<rocket::IPNetAddr>("127.0.0.1", 12345);
    rocket::TcpClient client(addr);

    client.connect([addr,&client]()
    {
        DEBUGLOG("connect to [%s] success",addr->toString().c_str());
        // printf("connect to [%s] success\n",addr->toString().c_str());
        std::shared_ptr<rocket::TinypbProtocol> message = std::make_shared<rocket::TinypbProtocol>();
        message->m_msg_id = "123456789";
        message->m_pb_data = "test pb data";
        client.writeMessage(message,[](rocket::AbstractProtocol::s_ptr msg_ptr)
        {
            printf("send messages success\n");
        });
        client.readMessage("123456789",[](rocket::AbstractProtocol::s_ptr msg_ptr)
        {
            std::shared_ptr<rocket::TinypbProtocol> message = std::dynamic_pointer_cast<rocket::TinypbProtocol>(msg_ptr);
            DEBUGLOG("req_id[%s], get response %s",message->m_msg_id.c_str(),message->m_pb_data.c_str());
        });
        client.writeMessage(message,[](rocket::AbstractProtocol::s_ptr msg_ptr)
        {
            printf("send messages 222222 success\n");
        });
    });
}

int main()
{
    rocket::Logger::InitGlobalLogger(2);

    // test_connect();

    test_tcp_client();

    return 0;
}