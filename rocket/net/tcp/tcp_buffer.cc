#include <vector>
#include <string.h>
#include <memory>
#include "rocket/net/tcp/tcp_buffer.h"
#include "rocket/common/log.h"
#include "tcp_buffer.h"

namespace rocket
{

}
rocket::TcpBuffer::TcpBuffer(int size) : m_size(size)
{
    m_buffer.resize(m_size);
}

rocket::TcpBuffer::~TcpBuffer()
{
}

int rocket::TcpBuffer::readAble()
{
    return m_write_index - m_read_index;
}

int rocket::TcpBuffer::writeAble()
{
    return m_buffer.size() - m_write_index;
}

int rocket::TcpBuffer::gerReadIndex()
{
    return m_read_index;
}

int rocket::TcpBuffer::getWriteIndex()
{
    return m_write_index;
}

void rocket::TcpBuffer::writeToBuffer(const char *buf, int size)
{
    // 当前写入的字节数 大于剩余的字节数
    if (size > writeAble())
    {
        // 调整buffer的大小  扩容
        int new_size = int(1.5 * (m_write_index + size));
        resizeBuffer(new_size);
    }
    memcpy(&m_buffer[m_write_index], buf, size);
    m_write_index+=size;
}

void rocket::TcpBuffer::readFromBuffer(std::vector<char> &re, int size)
{
    if (readAble() == 0)
    {
        return;
    }
    int read_size = readAble() > size ? size : readAble();

    std::vector<char> tmp(read_size);
    memcpy(&tmp[0], &m_buffer[m_read_index], read_size);
    re.swap(tmp);
    m_read_index += read_size;

    adjustBuffer();
}

void rocket::TcpBuffer::resizeBuffer(int new_size)
{
    std::vector<char> tmp(new_size);            // 新的大小
    int count = std::min(new_size, readAble()); // count记录的是原本buffer中有的元素

    memcpy(&tmp[0], &m_buffer[m_read_index], count); // 复制到新的容器中
    m_buffer.swap(tmp);                              // 交换

    m_read_index = 0;
    m_write_index = m_read_index + count;
}

//本质是数组的平移
void rocket::TcpBuffer::adjustBuffer()
{
    //readindex大于buffer长度的三分之一
    if(m_read_index<int(m_buffer.size()/3))
    {
        return ;
    }
    std::vector<char>buffer(m_buffer.size());
    int count = readAble();

    memcpy(&buffer[0],&m_buffer[m_read_index],count);
    m_buffer.swap(buffer);
    m_read_index=0;
    m_write_index = m_read_index+count;

    buffer.clear();
}

void rocket::TcpBuffer::moveReadIndex(int size)
{
    size_t j = m_read_index+size;
    if(j>=m_buffer.size())
    {
        ERRORLOG("moveReadIndex error,invalid size %d,old_read_index%d,buffer size %d",size,m_read_index,m_buffer.size());
        return;
    }
    m_read_index=j;
    adjustBuffer();
}

void rocket::TcpBuffer::moveWriteIndex(int size)
{
    size_t j = m_write_index + size;
    if(j>=m_buffer.size())
    {
        ERRORLOG("moveWriteIndex error,invalid size %d,old_read_index%d,buffer size %d",size,m_read_index,m_buffer.size());
        return;
    }
    m_write_index=j;
    adjustBuffer();
}
