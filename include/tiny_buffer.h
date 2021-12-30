#ifndef __BUFFER_H__
#define __BUFFER_H__

#include <cstddef>

class Buffer
{
public:
    Buffer();
    ~Buffer();

    size_t size() const;
    bool empty() const;
    char* begin() const;
    char* end() const;
    char* data() const;
    size_t space() const;

    void makeRoom();
    char *makeRoom(size_t len);
    void addSize(size_t len); 
    void consumed(size_t len);
    void clear();  //清空状态

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