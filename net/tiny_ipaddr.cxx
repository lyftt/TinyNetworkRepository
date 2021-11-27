#include "tiny_ipaddr.h"
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <cstring>
#include <iostream>

struct in_addr getHostByName(const std::string &host) 
{
    struct in_addr addr;
    char buf[1024];
    struct hostent hent;
    struct hostent *he = NULL;
    int herrno = 0;
    memset(&hent, 0, sizeof hent);

    int r = gethostbyname_r(host.c_str(), &hent, buf, sizeof buf, &he, &herrno);
    if (r == 0 && he && he->h_addrtype == AF_INET) {
        addr = *reinterpret_cast<struct in_addr *>(he->h_addr);
    } else {
        addr.s_addr = INADDR_NONE;
    }

    return addr;
}

Ip4Addr::Ip4Addr(const std::string &host, unsigned short port) 
{
    memset(&m_addr, 0, sizeof(m_addr));

    m_addr.sin_family = AF_INET;
    m_addr.sin_port = htons(port);

    if(host.size())
    {
        m_addr.sin_addr = getHostByName(host);
    }
    else{
        m_addr.sin_addr.s_addr = INADDR_ANY;
    }

    if(m_addr.sin_addr.s_addr == INADDR_NONE)
    {
        std::cout<<"can't resolve "<<host<<" to ip"<<std::endl;
    }
}

std::string Ip4Addr::toString() const
{
    std::string ipStr(inet_ntoa(m_addr.sin_addr));
    return ipStr + ":" + ntohs(m_addr.sin_port);
}

std::string Ip4Addr::ip() const
{
    return std::string(inet_ntoa(m_addr.sin_addr));
}

unsigned short Ip4Addr::port() const
{
    return (unsigned short)ntohs(m_addr.sin_port);
}

bool Ip4Addr::isIpValid() const
{
    return m_addr.sin_addr.s_addr != INADDR_NONE;
}