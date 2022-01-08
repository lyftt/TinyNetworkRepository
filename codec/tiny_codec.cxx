#include "tiny_codec.h"
#include <cstring>

//报文
//报文格式:
// |0x3f|报文长度|报文类型|crc校验|报文内容...|
//   1b     2b      2b       4b     nb
//
//
Buffer CommonCodec::encode(const Rpt& rpt)
{
    char temp[9] = {0};
    size_t offset = 0;

    //报文开头0x3f
    temp[0] = 0x3f;
    offset += 1;

    //报文长度
    memcpy(temp + offset, &rpt.m_rptHead.m_rptLen, sizeof(unsigned short));
    offset += sizeof(unsigned short);
    
    //报文类型
    memcpy(temp + offset, &rpt.m_rptHead.m_type, sizeof(unsigned short));
    offset += sizeof(unsigned short);

    //crc
    int c = 0;
    memcpy(temp + offset, &c, sizeof(int));
    
    return std::move(Buffer().append(temp, 9).append(rpt.m_rptBuffer.data(), rpt.m_rptBuffer.size()));
}

//返回类型tuple
//第一个元素:
// >0 解码出了一个报文，返回消费掉的字节数
// =0 没有解码出报文，数据不够
// <0 没有解码出报文，缓冲区数据有问题
//第二个元素
//
std::tuple<long, Rpt> CommonCodec::decode(char* buf, size_t len)
{
    //缓冲区数据有问题
    if(*buf != 0x3f)
    {
        return std::tuple<long, Rpt>(-1, Rpt());
    }

    size_t rptLen = *((unsigned short*)(buf + 1));   //报文长度
    unsigned short rptTy = *((unsigned short*)(buf + 3));    //报文类型
    int crc = *((int*)(buf + 5));   //crc校验

    //crc校验不正确，缓冲区数据有问题
    if(crc != 0x00)
    {
        return std::tuple<long, Rpt>(-1, Rpt());
    }

    //如果现有接收缓冲区的数据不够一个报文
    if(len < (1 + rptLen))
    {
        return std::tuple<long, Rpt>(0, Rpt());
    }

    Rpt rpt;
    rpt.m_rptHead.m_rptLen = rptLen;
    rpt.m_rptHead.m_type = rptTy;
    rpt.m_rptBuffer.append(buf + 9, rptLen - 8);  //包头8字节(长度+类型+CRC)

    return std::tuple<long, Rpt>(1 + rptLen, std::move(rpt));
}
