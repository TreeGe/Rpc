// tcp缓冲区类
// 主要是为了实现异步传输
#ifndef ROCKET_NET_TCP_BUFFER_H
#define ROCKET_NET_TCP_BUFFER_H

#include <vector>
#include <memory>

namespace rocket
{
    class TcpBuffer
    {
    public:
        typedef std::shared_ptr<TcpBuffer> s_ptr;

        TcpBuffer(int size);

        ~TcpBuffer();

        // 返回可读的字节数
        int readAble();

        // 返回可写的字节数
        int writeAble();

        // 返回读取的位置
        int gerReadIndex();

        // 返回写入的位置
        int getWriteIndex();

        // 写入数据
        void writeToBuffer(const char *buf, int size);

        // 读出数据
        void readFromBuffer(std::vector<char> &re, int size);

        // 扩容的函数
        void resizeBuffer(int new_size);

        // 调整 读指针与写指针的位置
        void adjustBuffer();

        void moveReadIndex(int size);

        void moveWriteIndex(int size);

    private:
        int m_read_index{0};  // 读取的位置
        int m_write_index{0}; // 写入的位置
        int m_size{0};        // 数组的大小/buffer的大小
    public:
        std::vector<char> m_buffer;
    };
}

#endif