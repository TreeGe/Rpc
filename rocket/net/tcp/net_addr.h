#ifndef ROCKET_NET_TCP_ADDR_H
#define ROCKET_NET_TCP_ADDR_H

#include<memory>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string>

namespace rocket
{
    class NetAddr
    {
    public:

        typedef std::shared_ptr<NetAddr> s_ptr;  //可以实现多态

        virtual sockaddr *getSockAddr() = 0;

        virtual socklen_t getSockLen() = 0;

        virtual int getFamily() = 0;

        virtual std::string toString() = 0;

        virtual bool checkVaild() = 0;
    };

    class IPNetAddr : public NetAddr
    {
    public:
        IPNetAddr(const std::string &ip, uint16_t port); // 传入ip 端口进行构造

        IPNetAddr(const std::string &addr); // 传入一个ip 端口的字符串格式进行构造

        IPNetAddr(sockaddr_in addr);

        sockaddr *getSockAddr();

        socklen_t getSockLen();

        int getFamily();

        std::string toString();

        bool checkVaild();   //检查当前地址的合法性

    private:
        std::string m_ip;
        uint16_t m_port{0};

        sockaddr_in m_addr;
    };

}

#endif
