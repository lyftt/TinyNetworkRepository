#include "tiny_tcpserver.h"
#include "tiny_ipaddr.h"
#include "tiny_tcpconnectionpool.h"
#include "tiny_tcpconnection.h"
#include "tiny_channel.h"
#include "tiny_poller.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <cstring>
#include <iostream>
#include <memory>
#include <unistd.h>

bool setNonBlocking(int sockfd) 
{    
    int nb = 1;  //0：清除，1：设置  

    if(ioctl(sockfd, FIONBIO, &nb) == -1) //FIONBIO：设置/清除非阻塞I/O标记：0：清除，1：设置
    {
        return false;
    }

    return true;
    /* 
    //fcntl:file control文件控制相关函数，执行各种描述符控制操作
    int opts = fcntl(sockfd, F_GETFL);  //用F_GETFL先获取描述符的一些标志信息
    if(opts < 0) 
    {
        return false;
    }

    opts |= O_NONBLOCK; //把非阻塞标记加到原来的标记上，标记这是个非阻塞套接字【如何关闭非阻塞呢？opts &= ~O_NONBLOCK,然后再F_SETFL一下即可】
    if(fcntl(sockfd, F_SETFL, opts) < 0) 
    {
        return false;
    }

    return true;
    */
}

struct TcpServer::TcpServerImpl
{
    TcpServerImpl(EventBase* base, const std::string& host, unsigned short port, int connPoolSize);
    ~TcpServerImpl();

    void init();

    enum STATUS
    {
        NOT_INIT,
        INITED,
        WORKING,
        FINISHED,
        FAILED
    };

private:
    void handleAccept();

private:
    EventBase*                         m_base;          //事件驱动
    Ip4Addr                            m_addr;          //服务器地址
    std::shared_ptr<TcpConnectionPool> m_connPoolPtr;   //服务器连接池
    int                                m_connPoolSize;  //服务器连接池大小
    Channel*                           m_listenChannel; //监听通道
    STATUS                             m_status;        //tcp服务器状态
};

TcpServer::TcpServerImpl::TcpServerImpl(EventBase* base, const std::string& host, unsigned short port, int connPoolSize):m_base(base), m_addr(host, port), m_status(NOT_INIT), m_connPoolSize(connPoolSize)
{

}

TcpServer::TcpServerImpl::~TcpServerImpl()
{

}

void TcpServer::TcpServerImpl::handleAccept()
{
    struct sockaddr_in clientAddr;
    socklen_t clientAddrSize = sizeof(clientAddr);
    int cfd = 0;
    int err = 0;
    int listenSocket = m_listenChannel->fd();

    //循环取出所有连接套接字
    do
    {
        cfd = accept4(listenSocket, (struct sockaddr*)&clientAddr, &clientAddrSize, SOCK_NONBLOCK);   //直接设置为非阻塞
        //cfd = accept(listenSocket, &clientAddr, &clientAddrSize);
        struct sockaddr_in peer, local;  //对端地址信息，本端地址信息
        socklen_t aLen = sizeof(peer);

        //判断错误情况，是真出错还是全连接队列中暂时没有连接可取
        if(cfd == -1)
        {
            err = errno;

            if(err == EAGAIN || err == EWOULDBLOCK)  //全连接队列中连接已经全部取走
            {
                std::cout<<" accept4 return EAGAIN "<< std::endl;
                return;
            }

            if(err == ECONNABORTED) //TCP 连接的“三次握手”后，客户TCP却发送了一个RST（复位）分节，在服务进程看来，就在该连接已由TCP排队，等着服务进程调用accept的时候RST却到达了
            {
                std::cout<<" accept return ECONNABORTED"<<std::endl;
                return;
            }

            if(err == ECONNRESET) //由于超时或者其它失败而中止接连(例如用户突然把网线、断电)
            {
                std::cout<<" accept return ECONNRESET"<<std::endl;
                return;
            }

            if(err == EMFILE) //进程打开的文件描述符达到上限
            {
                std::cout<<" accept return EMFILE"<<std::endl;
                return;
            }

            if(err == ENFILE) //系统打开的文件描述符达到了上限
            {
                std::cout<<" accept return ENFILE"<<std::endl;
                return;
            }

            if(err == ENOSYS) //该函数没有实现
            {
                std::cout<<" accept return ENOSYS"<<std::endl;
                return;
            }

            std::cout<<" accept return other errno"<<std::endl;
            return;
        }

        int ret = getpeername(cfd, (struct sockaddr *) &peer, &aLen);
        if(ret < 0)
        {
            std::cout<<" peer getpeername return error:" << strerror(errno) <<std::endl;
            continue;
        }

        ret = getsockname(cfd, (struct sockaddr *) &local, &aLen);
        if(ret < 0)
        {
            std::cout<<" local getsockname return error:" << strerror(errno) <<std::endl;
            continue;
        }

        TcpConnection* newConn = m_connPoolPtr->getOneTcpConnection(cfd, m_base, local, peer);
        if(newConn == nullptr) //连接池已满
        {
            std::cout<<" tcp connection pool is full now"<<std::endl;
            ::close(cfd);  //连接池中连接不够，关闭连接
            return;
        }

        //设置连接的读写回调函数
        newConn->onRead([](TcpConnection&) { std::cout <<"int tcp read task" << std::endl; });

    } while (1);
    
}

void TcpServer::TcpServerImpl::init()
{
    int listenSock = socket(AF_INET, SOCK_STREAM, 0);   //创建socket
    if(listenSock < 0)
    {
        std::cout<<"socket error:"<<strerror(errno)<<std::endl;
        m_status = FAILED;
        return;
    }

    int reUseAddr = 1;
    int ret = setsockopt(listenSock, SOL_SOCKET, SO_REUSEADDR, (const void *) &reUseAddr, sizeof(reUseAddr));
    if(ret < 0)
    {
        std::cout<<"socket setsockopt reuseaddr error:"<<strerror(errno)<<std::endl;
        m_status = FAILED;
        return;
    }

    if(!setNonBlocking(listenSock))
    {
        std::cout<<"socket setNonBlocking error:"<<strerror(errno)<<std::endl;
        m_status = FAILED;
        return;
    }

    ret = bind(listenSock, (struct sockaddr *) &m_addr.getAddr(), sizeof(struct sockaddr));
    if(ret < 0)
    {
        std::cout<<"socket bind error:"<<strerror(errno)<<std::endl;
        m_status = FAILED;
        return;
    }

    ret = listen(listenSock, 5000);
    if(ret < 0)
    {
        std::cout<<"socket listen error:"<<strerror(errno)<<std::endl;
        m_status = FAILED;
        return;
    }

    m_connPoolPtr = std::make_shared<TcpConnectionPool>(m_connPoolSize);  //创建连接池

    m_listenChannel = new Channel(m_base, listenSock, ReadEvent);  //为监听套接字创建通道，并监听读事件
    m_listenChannel->onRead([this] { handleAccept(); });   //设置监听套接字的回调函数
    m_status = INITED;
}

TcpServer::TcpServer(EventBase* base, const std::string& host, unsigned short port, int connPoolSize)
{
    m_impl.reset(new TcpServerImpl(base, host, port, connPoolSize));
    m_impl->init();
}

TcpServer::~TcpServer()
{

}