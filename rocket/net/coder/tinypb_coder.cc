#include <vector>
#include <string.h>
#include <memory>
#include <arpa/inet.h>
#include "rocket/net/coder/tinypb_coder.h"
#include "rocket/net/tcp/tcp_buffer.h"
#include "rocket/net/coder/tinypb_protocol.h"
#include "rocket/common/util.h"
#include "rocket/common/log.h"
#include "tinypb_coder.h"

namespace rocket
{
            //将message 对象转化为字节流  写入到buffer
    void TinypbCoder::encoder(std::vector<AbstractProtocol::s_ptr>& messages,TcpBuffer::s_ptr out_buffer)
    {
        for(auto it=messages.begin();it!=messages.end();it++)
        {
            std::shared_ptr<TinypbProtocol> msg = std::dynamic_pointer_cast<TinypbProtocol>(*it);
            int len=0;
            const char* buf = encodeTinypb(msg,len);
            if (buf != NULL && len != 0)
            {
                out_buffer->writeToBuffer(buf,len);
            }

            if(buf)
            {
                free((void*)buf);
                buf=NULL;
            }
        }

    }

        //将buffer里面的字节流转换为message对象
    void TinypbCoder::decoder(std::vector<AbstractProtocol::s_ptr>& messages,TcpBuffer::s_ptr buffer)
    {
        while(true)
        {
            //解码过程 遍历buffer 找到PB_START 找到之后 解析整包的长度。  然后得到结束符的位置 判断是否为PB_END
            std::vector<char>temp = buffer->m_buffer;
            int start_index = buffer->gerReadIndex();  //协议的起始位置
            int end_index = -1;                     

            int pk_len=0;

            bool parse_success = false;
            int i=0;
            for(i=start_index ;i < buffer->getWriteIndex(); i++)
            {
                if(temp[i]==TinypbProtocol::PB_START)
                {
                    //从下取四个字符  由于是网络字节序 需要转换为主机字节序
                    if(i+1 < buffer->getWriteIndex())
                    {
                        pk_len = getInt32FromNetByte(&temp[i+1]);
                        DEBUGLOG("get pk_len = %d",pk_len);


                        //结束符的索引
                        int j=i+pk_len-1;
                        if(j>=buffer->getWriteIndex())
                        {
                            continue;
                        }
                        if(temp[j]==TinypbProtocol::PB_END)
                        {
                            start_index = i;
                            end_index = j;
                            parse_success=true;
                            break;
                        }

                    }
                }
            }

            if(i>=buffer->getWriteIndex()) //没有数据可读
            {
                DEBUGLOG("decode end , read all buffer data");
                return;
            }

            if(parse_success)
            {
                buffer->moveReadIndex(end_index-start_index+1);  //移动读的位置
                std::shared_ptr<TinypbProtocol> message = std::make_shared<TinypbProtocol>();
                message->m_pk_len = pk_len;

                int req_id_len_index = start_index + sizeof(char) + sizeof(message->m_pk_len);   //获取req_len的位置
                if(req_id_len_index>=end_index)
                {
                    message->parse_success = false;
                    ERRORLOG("parse error , req_id_len_index [%d] >= end_index[%d]",req_id_len_index,end_index);
                    continue;
                }
                message->m_msg_id_len = getInt32FromNetByte(&temp[req_id_len_index]);  
                INFOLOG("parse req_id_len=%d",message->m_msg_id_len);

                int req_id_index = req_id_len_index + sizeof(message->m_msg_id_len); //获取req_id的位置

                char req_id[100] = {0};
                memcpy(&req_id[0],&temp[req_id_index],message->m_msg_id_len);
                message->m_msg_id = std::string(req_id);
                INFOLOG("parse req_id=%s",message->m_msg_id.c_str());


                int method_name_len_index = req_id_index + message->m_msg_id_len;  //获取方法名长度的位置
                if(method_name_len_index>=end_index)
                {
                    message->parse_success = false;
                    ERRORLOG("parse error , req_method_len_index [%d] >= end_index[%d]",method_name_len_index,end_index);
                    continue;
                }
                message->m_method_name_len = getInt32FromNetByte(&temp[method_name_len_index]);

                int method_name_index = method_name_len_index + sizeof(message->m_method_name_len);
                char method_name[512] = {0};
                memcpy(&method_name[0],&temp[method_name_index],message->m_method_name_len);
                message->m_method_name = std::string(method_name);
                ERRORLOG("parse method_name=%s",message->m_method_name.c_str());

                int err_code_index = method_name_index + message->m_method_name_len;
                if(err_code_index>=end_index)
                {
                    message->parse_success = false;
                    ERRORLOG("parse error , err_code_index [%d] >= end_index[%d]",err_code_index,end_index);
                    continue;
                }
                message->m_err_code = getInt32FromNetByte(&temp[err_code_index]);

                int error_info_len_index = err_code_index + sizeof(message->m_err_code);
                if(error_info_len_index>=end_index)
                {
                    message->parse_success = false;
                    ERRORLOG("parse error , error_info_len_index [%d] >= end_index[%d]",error_info_len_index,end_index);
                    continue;
                }
                message->m_err_info_len = getInt32FromNetByte(&temp[error_info_len_index]);
                
                //获取error_info
                int error_info_index = error_info_len_index +  sizeof(message->m_err_info_len);
                char error_info[512] = {0};
                memcpy(&error_info[0],&temp[error_info_index],message->m_err_info_len);
                message->m_err_info = std::string(error_info);
                ERRORLOG("parse error_info=%s\n",message->m_err_info.c_str());

                //获取protobuf序列的长度
                int pb_data_len = message->m_pk_len - message->m_msg_id_len - message->m_method_name_len - message->m_err_info_len
                                    - sizeof(message->PB_START) - sizeof(message->PB_END) - sizeof(message->m_msg_id_len) 
                                    - sizeof(message->m_method_name_len) - sizeof(message->m_err_code) - sizeof(message->m_err_info_len)
                                    -sizeof(message->m_pk_len) - sizeof(message->m_check_num);
                
                int pb_data_index = error_info_index + message->m_err_info_len;
                message->m_pb_data = std::string(&temp[pb_data_index],pb_data_len);

                //这里校验和取解析
                message->parse_success = true;

                messages.push_back(message);

            }
        }

    }

    const char *rocket::TinypbCoder::encodeTinypb(std::shared_ptr<TinypbProtocol> message, int &len)
    {
        if(message->m_msg_id.empty())
        {
            message->m_msg_id = "123456789";
        }
        DEBUGLOG("req_id=%s\n",message->m_msg_id.c_str());
        int pk_len = 2 + 24 + message->m_msg_id.length() + message->m_method_name.length() + message->m_pb_data.length() + message->m_err_info.length();
        DEBUGLOG("pk_len = %d\n",pk_len);

        char* buf = reinterpret_cast<char*>(malloc(pk_len));
        char* temp=buf;

        *temp = TinypbProtocol::PB_START;
        temp++;

        int32_t pk_len_net = htonl(pk_len);
        memcpy(temp,&pk_len_net,sizeof(pk_len_net));
        temp+=sizeof(pk_len_net);
        
        int req_id_len = message->m_msg_id.length();
        int32_t req_id_len_net = htonl(req_id_len);
        memcpy(temp,&req_id_len_net,sizeof(req_id_len_net));
        temp+=sizeof(req_id_len_net);

        if(!message->m_msg_id.empty())
        {
            memcpy(temp,&(message->m_msg_id[0]),req_id_len);
            temp+=req_id_len;
        }


        int method_name_len = message->m_method_name.length();
        int32_t method_name_len_net = htonl(method_name_len);
        memcpy(temp,&method_name_len_net,sizeof(method_name_len_net));
        temp+=sizeof(method_name_len_net);
        
        if(!message->m_method_name.empty())
        {
            memcpy(temp,&(message->m_method_name[0]),method_name_len);
            temp += method_name_len;
        }

        int error_code_net = htonl(message->m_err_code);
        memcpy(temp,&error_code_net,sizeof(error_code_net));
        temp+=sizeof(error_code_net);

        int error_info_len = message->m_err_info.length();
        int32_t error_info_len_net = htonl(error_info_len);
        memcpy(temp,&error_info_len_net,sizeof(error_info_len_net));
        temp+=sizeof(error_info_len_net);
        
        if(!message->m_err_info.empty())
        {
            memcpy(temp,&(message->m_err_info[0]),error_info_len);
            temp += error_info_len;
        }

        if(!message->m_pb_data.empty())
        {
            memcpy(temp,&(message->m_pb_data[0]),message->m_pb_data.length());
            temp += message->m_pb_data.length();
        }

        int32_t check_sum_net = htonl(1);
        memcpy(temp,&check_sum_net,sizeof(check_sum_net));
        temp+=sizeof(check_sum_net);


        *temp = TinypbProtocol::PB_END;

        message->m_pk_len = pk_len;
        message->m_msg_id_len = req_id_len;
        message->m_method_name_len=method_name_len;
        message->m_err_info_len = error_info_len;
        message->parse_success = true;
        len = pk_len;

        DEBUGLOG("encoder message[%s] success",message->m_msg_id.c_str());

        //任务将上面错做封装成一个宏

        return buf;
    }
}
