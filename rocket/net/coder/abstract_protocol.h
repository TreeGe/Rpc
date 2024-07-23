#ifndef ROCKET_NET_ABSTRACT_PROTOCOL_H
#define ROCKET_NET_ABSTRACT_PROTOCOL_H

#include <memory>
#include "rocket/net/tcp/tcp_buffer.h"

namespace rocket
{
    struct AbstractProtocol
    {
        public:
        typedef std::shared_ptr<AbstractProtocol>s_ptr;
        
        virtual ~AbstractProtocol(){};
        std::string m_msg_id;   //请求号  唯一标识一个请求或者相应
    };
}

#endif