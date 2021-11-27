#ifndef __IP_ADDR_H__
#define __IP_ADDR_H__

#include <string>
#include <netinet/in.h>

struct Ip4Addr
{
    Ip4Addr(const std::string &host, unsigned short port);
    Ip4Addr(unsigned short port = 0):Ip4Addr("", port){}     //委托构造函数
    Ip4Addr(const struct sockaddr_in &addr) : m_addr(addr){};

    std::string toString() const;
    std::string ip() const;
    unsigned short port() const;
    bool isIpValid() const;

    struct sockaddr_in& getAddr(){
        return m_addr;
    }

    static std::string hostToIp(const std::string &host) {
        Ip4Addr addr(host, 0);
        return addr.ip();
    }

private:
    struct sockaddr_in  m_addr;
};

#endif