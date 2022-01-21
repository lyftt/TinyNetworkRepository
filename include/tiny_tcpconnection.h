#ifndef __TCP_CONNECTION_H__
#define __TCP_CONNECTION_H__

#include "tiny_ipaddr.h"
#include "tiny_util.h"
#include "tiny_buffer.h"
#include <unistd.h>
#include <atomic>

struct Channel;
struct EventBase;
struct TcpConnectionPool;

//连接对象
struct TcpConnection
{
    TcpConnection();
    ~TcpConnection();

    void init(int fd, EventBase* base, Ip4Addr local, Ip4Addr peer, TcpConnectionPool*);
    void reset();
    void close();

    void onRead(TcpTask& readTask) { m_readTask = readTask; }
    void onRead(TcpTask&& readTask) { m_readTask = std::move(readTask); }
    void onWrite(TcpTask& writeTask) { m_writeTask = writeTask; }
    void onWrite(TcpTask&& writeTask) { m_writeTask = std::move(writeTask); }

    void tcpHandleRead();
    void tcpHandleWrite();

    size_t send(Buffer& buffer);
    size_t isend(const char* buf, size_t len);

    int readImp(int fd, void *buf, size_t bytes) { return ::read(fd, buf, bytes); }
    int writeImp(int fd, const void *buf, size_t bytes) { return ::write(fd, buf, bytes); }

    char*  getRecvBufferAddr(); //获取接收缓冲地址
    size_t getRecvBufferSize(); //获取接收缓冲大小
    void   consumedRecvBufferSize(size_t len); //从接收缓冲区消费掉len的数据

    bool useSendEvent(){ return m_useSendEvent.load(); }
    uint64_t curSequence() { return m_curSequence.load(); }

    EventBase*     m_base;         //事件驱动器
    int            m_fd;           //文件描述符
    Channel*       m_channel;      //TCP通道
    std::atomic<uint64_t>  m_curSequence;  //用于检测废包
    std::atomic<bool>  m_useSendEvent;     //是否使用发送事件来驱动发送
    Ip4Addr        m_local;        //地址信息
    Ip4Addr        m_peer;         //地址信息
    TcpTask        m_readTask;     //读回调函数
    TcpTask        m_writeTask;    //写回调函数
    Buffer         m_recvBuffer;   //TCP接收缓冲
    Buffer         m_writeBuffer;  //TCP写缓冲区
    TcpConnectionPool* m_connPool; //TCP连接池
};

#endif