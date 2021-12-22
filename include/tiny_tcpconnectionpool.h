#ifndef __TCP_CONNECTION_POOL_H__
#define __TCP_CONNECTION_POOL_H__

#include <mutex>
#include "tiny_ipaddr.h"
#include "tiny_util.h"
#include <list>
struct TcpConnection;
struct EventBase;

//连接池对象
class TcpConnectionPool : private std::mutex, private NonCopyable
{
public:
    TcpConnectionPool(int size);
    ~TcpConnectionPool();

    int  getSize() const;                         //获取连接池大小
    int  getFreeSize();                           //获取空闲连接大小
    void init();                                  //初始化连接池
    TcpConnection* getOneTcpConnection(int fd, EventBase* base, Ip4Addr local, Ip4Addr peer);   //从连接池中获取一条连接
    void           putOneTcpConnection(TcpConnection* tcpConn);                        //向连接池中回收一条连接

private:
    char*                     m_poolStartMem;         //连接池起始地址
    int                       m_totalSize;           
    std::list<TcpConnection*> m_freeConnectionList;   //空闲的连接的队列
};

#endif