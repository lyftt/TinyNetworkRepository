#ifndef __CODEC_H__
#define __CODEC_H__

#include <cstddef>
#include <tiny_buffer.h>

struct MsgHead
{
    unsigned short m_msgLen;  //消息长度(包头+包体)
    unsigned short m_type;    //消息类型
    int m_crc32;
};

struct Msg
{
    MsgHead m_msgHead;
    Buffer  m_msgBuffer;
};

struct CodeBase
{
    virtual Buffer encode(const Msg& msg) = 0;     //编码消息
    virtual Msg decode(char* buf, size_t len) = 0; //解码消息
    virtual ~CodeBase() {}
};

struct CommonCodec:public CodeBase
{
    Buffer encode(const Msg& msg);
    Msg decode(char* buf, size_t len);

private:

};

#endif