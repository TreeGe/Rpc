#ifndef ROCKET_NET_ABSTRACT_CODER_H
#define ROCKET_NET_ABSTRACT_CODER_H

#include <vector>
#include <memory>
#include "rocket/net/tcp/tcp_buffer.h"
#include "rocket/net/coder/abstract_protocol.h"

namespace rocket
{
    class AbstractCoder
    {
        public:
        // std::shared_ptr<AbstractCoder> s_ptr;
        //将message 对象转化为字节流  写入到buffer
        virtual void encoder(std::vector<AbstractProtocol::s_ptr>& messages,TcpBuffer::s_ptr out_buffer)=0;

        //将buffer里面的字节流转换为message对象
        virtual void decoder(std::vector<AbstractProtocol::s_ptr>& messages,TcpBuffer::s_ptr out_buffer)=0;

        virtual ~AbstractCoder() {}
    };
}

#endif