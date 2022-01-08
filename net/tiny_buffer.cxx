#include "tiny_buffer.h"
#include <algorithm>
#include <cstring>

Buffer::Buffer():m_buf(nullptr), m_begin(0), m_end(0), m_capacity(0), m_exp(512)
{

}

Buffer::~Buffer()
{
    delete[] m_buf;
}

Buffer::Buffer(Buffer&& b):m_buf(b.m_buf), m_begin(b.m_begin), m_end(b.m_end), m_capacity(b.m_capacity), m_exp(b.m_exp)
{
    b.clear(); //清空原来的
}

Buffer& Buffer::operator=(Buffer&& b)
{
    if(&b == this)
    {
        return *this;
    }

    //先清空自己
    this->clear();

    //移动
    m_buf = b.m_buf;
    m_begin = b.m_begin;
    m_end = b.m_end;
    m_capacity = b.m_capacity;

    //清空b
    b.clear();  

    return *this;
}

size_t Buffer::size() const
{
    return m_end - m_begin;
}

bool Buffer::empty() const
{
    return m_begin == m_end;
}

char* Buffer::begin() const
{
    return m_buf + m_begin;
}

char* Buffer::end() const
{
    return m_buf + m_end;
}

char* Buffer::data() const
{
    return m_buf + m_begin;
}

size_t Buffer::space() const
{
    return m_capacity - m_end;
}

void Buffer::makeRoom()
{
    if(space() < m_exp)
        expand(0);
}

void Buffer::addSize(size_t len)
{
    m_end += len;
}

void Buffer::expand(size_t len)
{
    size_t nCap = std::max(m_exp, std::max(m_capacity * 2, size() + len));
    char* p = new char[nCap];
    std::copy(begin(), end(), p);

    m_end -= m_begin;
    m_begin = 0;
    delete[] m_buf;
    m_buf = p;
    m_capacity = nCap;
}

void Buffer::clear()
{
    delete[] m_buf;
    m_buf = nullptr;
    m_begin = 0;
    m_end = 0;
    m_capacity = 0;
}

void Buffer::consumed(size_t len)
{
    m_begin += len;
    if(size() == 0)
        clear();
}

void Buffer::moveHead()
{
    std::copy(begin(), end(), m_buf);
    m_begin = 0;
    m_end -= m_begin;
}

//返回尾部
char *Buffer::makeRoom(size_t len) 
{
    if (m_begin + len <= m_capacity) {
    } else if (size() + len < m_capacity / 2) {
        moveHead();  //向前紧凑
    } else {
        expand(len);
    }
    return end();
}

Buffer& Buffer::append(const char* data, size_t len)
{
    memcpy(makeRoom(len), data, len);
    addSize(len);
    return *this;
}