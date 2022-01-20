#include "tiny_tcpconnection.h"
#include "tiny_channel.h"
#include "tiny_poller.h"
#include "tiny_tcpconnectionpool.h"
#include <iostream>

TcpConnection::TcpConnection():m_channel(nullptr), m_base(nullptr), m_useSendEvent(false)
{
    m_curSequence = 0;
}

TcpConnection::~TcpConnection()
{

}

void TcpConnection::init(int fd, EventBase* base, Ip4Addr local, Ip4Addr peer, TcpConnectionPool* pool)
{
    ++m_curSequence;
    m_local = local;
    m_peer = peer;
    m_base = base;
    m_fd = fd;
    m_connPool = pool;
    m_useSendEvent = false;

    m_channel = new Channel(base, fd, ReadEvent); //一开始只允许监听读事件
    m_channel->onRead([=] { tcpHandleRead(); });  //给通道设置读回调函数
    m_channel->onWrite([=] { tcpHandleWrite(); }); //给通道设置写回调函数
}

void TcpConnection::reset()
{
    ++m_curSequence;
    m_base = nullptr;
    m_fd = -1;
    m_connPool = nullptr;
    m_useSendEvent = false;

    if (m_channel)
    {
        delete m_channel;
        m_channel = nullptr;
    }
}

//主动关闭连接
void TcpConnection::close()
{
    m_connPool->putOneTcpConnection(this);
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

void TcpConnection::tcpHandleWrite()
{
    if(m_writeBuffer.size() > 0)
    {
        size_t sended = isend(m_writeBuffer.data(), m_writeBuffer.size());
        m_writeBuffer.consumed(sended);

        //数据已经全部发完，需要取消事件发送驱动
        if(m_writeBuffer.empty() && m_channel->writeEnabled())
        {
            m_channel->enableWrite(false);
            m_useSendEvent.store(false);
        }
    }
}

//返回直接发送出去的字节数
size_t TcpConnection::isend(const char* buf, size_t len)
{
    size_t nsend = 0;

    while(len > nsend)
    {
        size_t wd = writeImp(m_channel->fd(), buf + nsend, len - nsend);

        if(wd > 0)
        {
            nsend += wd;
            continue;
        }
        //被信号中断
        else if(wd < 0 && errno == EINTR)
        {
            continue;
        }
        //发送缓冲区满
        else if(wd < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
        {
            if(!m_channel->writeEnabled())
            {
                m_useSendEvent = true;
                m_channel->enableRead(true); //使能事件驱动
            }
            break;
        }
        //发送出错
        else
        {
            break;
        }
    }

    return nsend;
}

//进行数据发送，需要处理发送满的情况
//返回0 ：直接发送完
//返回1 ：未发送完，启动了事件驱动
//返回-1：已经启动了事件驱动暂时不能发送
size_t TcpConnection::send(Buffer& buffer)
{
    if(m_channel)
    {
        //如果当前已经在使用事件驱动进行发送
        if(m_useSendEvent.load())
        {
            return -1;
        }

        //开始直接发送数据
        if(buffer.size() > 0)
        {
            size_t sended = isend(buffer.data(), buffer.size());
            buffer.consumed(sended);
        }
        
        //还有数据没发送完，已经启动事件驱动了
        if(buffer.size() > 0)
        {
            m_writeBuffer.append(buffer.data(), buffer.size());
            return 1; 
        }
    }

    return 0;
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


