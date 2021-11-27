#include "tiny_tcpconnection.h"
#include "tiny_channel.h"
#include "tiny_poller.h"

TcpConnection::TcpConnection():m_channel(nullptr), m_base(nullptr)
{
    m_curSequence = 0;
}

TcpConnection::~TcpConnection()
{

}

void TcpConnection::init(int fd, EventBase* base, Ip4Addr local, Ip4Addr peer)
{
    m_curSequence++;
    m_local = local;
    m_peer = peer;
    m_base = base;

    m_channel = new Channel(base, fd, ReadEvent);
    m_channel->onRead([=] { tcpHandleRead(this); });  //给通道设置读回调函数
}

void TcpConnection::reset()
{
    m_curSequence++;
    m_base = nullptr;

    if (m_channel)
    {
        delete m_channel;
        m_channel = nullptr;
    }
}

void TcpConnection::tcpHandleRead(TcpConnection* tcpConn)
{
    //从通道中读取数据

    //在这里调用TcpConnection的读回调函数
}

void TcpConnection::tcpHandleWrite(TcpConnection* tcpConn)
{
    //向通道发送数据

    //在这里调用TcpConnection的写回调函数
}
