#ifndef __TCP_SERVER_H__
#define __TCP_SERVER_H__

#include <memory>
#include <string>
#include "tiny_util.h"
struct EventBase;

struct TcpServer
{
    TcpServer(EventBase* base, const std::string& host, unsigned short port, int connPoolSize=10000);
    ~TcpServer();

    void OnRead(TcpTask& task);
    void OnRead(TcpTask&& task);

private:
    struct TcpServerImpl;
    std::unique_ptr<TcpServerImpl>  m_impl;
};

#endif