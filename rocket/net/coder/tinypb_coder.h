#ifndef ROCKET_TINYPB_CODER_H
#define ROCKET_TINYPB_CODER_H

#include "rocket/net/coder/abstract_coder.h"
#include "rocket/net/coder/abstract_protocol.h"
#include "rocket/net/tcp/tcp_buffer.h"
#include "rocket/net/coder/tinypb_protocol.h"

namespace rocket
{
    class TinypbCoder : public AbstractCoder
    {
        public:
        // std::shared_ptr<AbstractCoder> s_ptr;
        //将message 对象转化为字节流  写入到buffer
        void encoder(std::vector<AbstractProtocol::s_ptr>& messages,TcpBuffer::s_ptr out_buffer);

        //将buffer里面的字节流转换为message对象
        void decoder(std::vector<AbstractProtocol::s_ptr>& messages,TcpBuffer::s_ptr out_buffer);

        TinypbCoder(){}
        ~TinypbCoder(){}

        private:
        const char* encodeTinypb(std::shared_ptr<TinypbProtocol>message,int &len);

    };
}


#endif