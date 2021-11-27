#include "tiny_tcpconnectionpool.h"
#include "tiny_tcpconnection.h"
#include <memory>

TcpConnectionPool::TcpConnectionPool(int size):m_poolStartMem(nullptr), m_totalSize(size)
{
    int totalSize = sizeof(TcpConnection) * size;
    void* p = new char[totalSize];
    TcpConnection* tcpConnPtrStart = (TcpConnection*)p;
    TcpConnection* tcpConnPtr = tcpConnPtrStart;

    for(int i = 0; i < size; ++i)
    {
        tcpConnPtr = new (tcpConnPtr) TcpConnection;
        m_freeConnectionList.push_back(tcpConnPtr);
        ++tcpConnPtr;
    }
}

TcpConnectionPool::~TcpConnectionPool()
{
    if(m_poolStartMem)
    {
        delete[] m_poolStartMem;
    }
}

int TcpConnectionPool::getSize() const
{
    return m_totalSize;
}

int TcpConnectionPool::getFreeSize() const
{
    return m_freeConnectionList.size();
}

//取出一条可用连接
TcpConnection* TcpConnectionPool::getOneTcpConnection(int fd, EventBase* base, Ip4Addr local, Ip4Addr peer)
{
    std::lock_guard<std::mutex> guard(*this);
    if(m_freeConnectionList.empty()) return nullptr;

    TcpConnection* temp = m_freeConnectionList.front();
    m_freeConnectionList.pop_front();
    temp->init(fd, base, local, peer);
    return temp;
}

//返回一条连接
void TcpConnectionPool::putOneTcpConnection(TcpConnection* tcpConn)
{
    std::lock_guard<std::mutex> guard(*this);
    if(tcpConn == nullptr) return;

    tcpConn->reset();
    m_freeConnectionList.push_back(tcpConn);
}

void TcpConnectionPool::init()
{

}