#ifndef __CODEC_H__
#define __CODEC_H__

#include <cstddef>
#include <tiny_buffer.h>
#include <cstdint>
#include <tuple>

 struct TcpConnection;

//报文头
struct RptHead
{
    unsigned short m_rptLen;  //报文长度(包头+包体)
    unsigned short m_type;    //报文类型
};

//报文
//报文格式:
// |0x3f|报文长度|报文类型|crc校验|报文内容...|
//   1b     2b      2b       4b     nb
//
//
struct Rpt
{
    RptHead m_rptHead;   //报文头
    Buffer  m_rptBuffer; //报文体
};

//消息头
struct MsgHead
{
    uint64_t m_curSeq;        //用于检错废包
    TcpConnection* m_tcpConn; //所属Tcp连接
};

//消息
struct Msg
{
    MsgHead m_msgHead;  //消息头
    Rpt     m_rpt;      //报文
};


struct CodeBase
{
    virtual Buffer encode(const Rpt& msg) = 0;     //编码消息
    virtual std::tuple<long, Rpt> decode(char* buf, size_t len) = 0; //解码消息
    virtual ~CodeBase() {}
};

struct CommonCodec:public CodeBase
{
    Buffer encode(const Rpt& rpt);
    std::tuple<long, Rpt> decode(char* buf, size_t len);

private:

};

#endif