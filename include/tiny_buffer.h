#ifndef __BUFFER_H__
#define __BUFFER_H__

#include <cstddef>

class Buffer
{
public:
    Buffer();
    ~Buffer();
    
    //只允许移动
    Buffer(Buffer&& b);
    Buffer& operator=(Buffer&& b);

    //禁止拷贝
    Buffer(const Buffer& b) = delete; 
    Buffer& operator=(const Buffer& b) =delete;

    size_t size() const;
    bool empty() const;
    char* begin() const;
    char* end() const;
    char* data() const;
    size_t space() const;

    void makeRoom();             //保证至少有512字节
    char *makeRoom(size_t len);  //保证至少有len的空间,返回尾部地址
    void addSize(size_t len);    //缓冲区增加len大小
    void consumed(size_t len);   //缓冲区消费掉len大小
    void clear();                //清空状态

    Buffer& append(const char* data, size_t len);   //从尾部增加len字节

private:
    void moveHead();
    void expand(size_t len);

private:
    char*  m_buf;      //缓冲起始
    size_t m_begin;    //数据的起始位置
    size_t m_end;      //数据的结束位置
    size_t m_capacity; //目前缓冲的空间总大小
    size_t m_exp;
};

#endif