#include <string.h>
#include "rocket/net/tcp/net_addr.h"
#include "rocket/common/log.h"
#include "net_addr.h"

namespace rocket
{

}
rocket::IPNetAddr::IPNetAddr(const std::string &ip, uint16_t port) : m_ip(ip), m_port(port)
{
    memset(&m_addr, 0, sizeof(m_addr));

    m_addr.sin_family = AF_INET;
    m_addr.sin_addr.s_addr = inet_addr(m_ip.c_str());
    m_addr.sin_port = htons(m_port);
}

rocket::IPNetAddr::IPNetAddr(const std::string &addr)
{
    size_t i = addr.find_first_of(":");
    if (i == addr.npos)
    {
        ERRORLOG("invalid ipv4 addr %s", addr.c_str());
        return;
    }
    m_ip = addr.substr(0, i);
    m_port = std::atoi(addr.substr(i + 1, addr.size() - i - 1).c_str());

    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sin_family = AF_INET;
    m_addr.sin_addr.s_addr = inet_addr(m_ip.c_str());
    m_addr.sin_port = htons(m_port);
}

rocket::IPNetAddr::IPNetAddr(sockaddr_in addr) : m_addr(addr)
{

    m_ip = std::string(inet_ntoa(addr.sin_addr));
    m_port = ntohs(m_addr.sin_port);
}
sockaddr *rocket::IPNetAddr::getSockAddr()
{
    return reinterpret_cast<sockaddr *>(&m_addr);
}
socklen_t rocket::IPNetAddr::getSockLen()
{
    return sizeof(m_addr);
}
int rocket::IPNetAddr::getFamily()
{
    return AF_INET;
}

std::string rocket::IPNetAddr::toString()
{
    std::string re;
    re = m_ip + ":" + std::to_string(m_port);
    return re;
}

bool rocket::IPNetAddr::checkVaild()
{
    if(m_ip.empty()) // ip地址为空
    {
        return false;
    }
    
    if(m_port < 0 || m_port > 65536) //port地址小于零 或者65536（端口使用的是无符号短整型unsgin short）
    {
        return false;
    }

    if(inet_addr(m_ip.c_str())==INADDR_NONE)  //inet_addr将字符串转为ip地址
    {
        return false;
    }
    return true;
}