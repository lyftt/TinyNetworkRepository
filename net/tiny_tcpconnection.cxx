#include "tiny_tcpconnection.h"
#include "tiny_channel.h"
#include "tiny_poller.h"
#include "tiny_tcpconnectionpool.h"
#include <iostream>

TcpConnection::TcpConnection():m_channel(nullptr), m_base(nullptr)
{
    m_curSequence = 0;
}

TcpConnection::~TcpConnection()
{

}

void TcpConnection::init(int fd, EventBase* base, Ip4Addr local, Ip4Addr peer, TcpConnectionPool* pool)
{
    m_curSequence++;
    m_local = local;
    m_peer = peer;
    m_base = base;
    m_fd = fd;
    m_connPool = pool;

    m_channel = new Channel(base, fd, ReadEvent);
    m_channel->onRead([=] { tcpHandleRead(); });  //给通道设置读回调函数
}

void TcpConnection::reset()
{
    m_curSequence++;
    m_base = nullptr;
    m_fd = -1;
    m_connPool = nullptr;

    if (m_channel)
    {
        delete m_channel;
        m_channel = nullptr;
    }
}

void TcpConnection::tcpHandleRead()
{
    //循环从通道中读取数据
    while(true)
    {
        m_recvBuffer.makeRoom();  //尝试扩大空间,保证至少有512字节空间可用
        int rd = readImp(m_channel->m_fd, m_recvBuffer.end(), m_recvBuffer.space());
    
        if(rd == 0)
        {
            //连接断开，连接池回收连接
            m_connPool->putOneTcpConnection(this);
            std::cout<<"connection break, give back to connection pool"<<std::endl;
            return;
        }
        else
        {
            if(rd < 0 && errno == EINTR)
            {
                continue; //读取操作被中断
            }
            else if(rd < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
            {
                break;  //暂时无数据可读
            }
            else if(rd < 0)
            {
                //连接有问题需要断开，连接池回收连接
                m_connPool->putOneTcpConnection(this);
                std::cout<<"connection break, give back to connection pool"<<std::endl;
                return;
            }
            else
            {
                m_recvBuffer.addSize(rd);
                std::cout<<"now recv buffer:"<<m_recvBuffer.size()<<std::endl;
            }
        }
    }
    
    //在这里调用TcpConnection的读回调函数
    if(m_readTask)
    {
        m_readTask(*this);
    }
}

char* TcpConnection::getRecvBufferAddr()
{
    return m_recvBuffer.data();
}

size_t TcpConnection::getRecvBufferSize()
{
    return m_recvBuffer.size();
}

void TcpConnection::consumedRecvBufferSize(size_t len)
{
    m_recvBuffer.consumed(len);
}


