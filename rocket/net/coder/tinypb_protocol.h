#ifndef ROCKET_TINYPB_PROTOCOL_H
#define ROCKET_TINYPB_PROTOCOL_H

#include <memory>
#include "rocket/net/coder/abstract_protocol.h"

namespace rocket
{

    struct TinypbProtocol : public AbstractProtocol
    {
        public:
        TinypbProtocol(){}
        ~TinypbProtocol(){}
        static char PB_START;   //协议的开始符
        static char PB_END;    //协议的结束符


        public:
        int32_t m_pk_len{0};      //包的总长度
        int32_t m_msg_id_len {0}; //请求号的长度
        //req_id继承父类
        int32_t m_method_name_len {0};  //方法名长度
        std::string m_method_name; //方法名
        int32_t m_err_code{0};     //错误码
        int32_t m_err_info_len{0}; //错误码信息长度
        std::string m_err_info;    //错误码信息长度
        std::string m_pb_data;     //protobuf序列化数据
        int32_t m_check_num{0};    //校验码

        bool parse_success {false};
    };

}


#endif