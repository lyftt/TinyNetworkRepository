#ifndef __POLLER_H__
#define __POLLER_H__

#include "tiny_util.h"
#include "tiny_channel.h"
#include <set>
#include <sys/epoll.h>
#include <atomic>

const int MaxEvents = 2000;
const int ReadEvent = EPOLLIN;
const int WriteEvent = EPOLLOUT;

// 抽象类
struct PollerBase : private NonCopyable
{
    int m_id;
    int m_lastActive;

    PollerBase():m_lastActive(-1){
        static std::atomic<int> id(0);
        m_id = ++id;
    };

    virtual ~PollerBase() override {};

    virtual void addChannel(Channel* ch) = 0;
    virtual void removeChannel(Channel* ch) = 0;
    virtual void updateChannel(Channel* ch) = 0;
    virtual void loopOnce(int waitMs) = 0;
};

// Epoll poller
struct EpollPoller: public PollerBase
{
    int m_epollFd;
    std::set<Channel*> m_liveChannels;
    struct epoll_event m_activeEvs[MaxEvents];

    EpollPoller();
    virtual ~EpollPoller();

    virtual void addChannel(Channel* ch);
    virtual void removeChannel(Channel* ch);
    virtual void updateChannel(Channel* ch);
    virtual void loopOnce(int waitMs);
};

PollerBase* createPoller();

#endif