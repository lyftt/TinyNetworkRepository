#include "tiny_poller.h"
#include <iostream>
#include <errno.h>
#include <cstring>
#include <unistd.h>

PollerBase* createPoller()
{
    return new EpollPoller;
}

EpollPoller::EpollPoller()
{
    m_epollFd = epoll_create1(EPOLL_CLOEXEC);
    if(m_epollFd < 0)
    {
        std::cout<<"epoll_create1 failed, errno:"<<errno<<" "<<strerror(errno);
    }
}

EpollPoller::~EpollPoller()
{
    while(m_liveChannels.size())
    {
        (*m_liveChannels.begin())->close();
    }

    ::close(m_epollFd);
}

void EpollPoller::addChannel(Channel* ch)
{
    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.events = ch->events();
    ev.data.ptr = ch;

    int ret = epoll_ctl(m_epollFd, EPOLL_CTL_ADD, ch->fd(), &ev);
    if(ret < 0)
    {
        std::cout<<"epoll_ctl failed, errno:"<<errno<<" "<<strerror(errno)<<std::endl;
        return;
    }

    m_liveChannels.insert(ch);
}

void EpollPoller::removeChannel(Channel* ch)
{
    m_liveChannels.erase();

    for(int i = m_lastActive; i >= 0; --i)
    {
        if(ch == m_activeEvs[i].data.ptr)
        {
            m_activeEvs[i].data.ptr = NULL;
            break;
        }
    }
}

void EpollPoller::updateChannel(Channel* ch)
{
    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.events = ch->events();
    ev.data.ptr = ch;

    int ret = epoll_ctl(m_epollFd, EPOLL_CTL_MOD, ch->fd(), &ev);   //epoll_ctl修改的时候, ev中的事件必须全部配置，如果不配置，之前的会被覆盖掉
    if(ret < 0)
    {
        std::cout<<"epoll_ctl failed, errno:"<<errno<<" "<<strerror(errno)<<std::endl;
        return;
    }
}

void EpollPoller::loopOnce(int waitMs)
{
    m_lastActive = epoll_wait(m_epollFd, m_activeEvs, MaxEvents, waitMs);

    if(m_lastActive < 0)
    {
        if(errno == EINTR)
        {
            return;
        }

        std::cout<<"epoll_wait failed, errno:"<<m_lastActive<<" "<<strerror(errno)<<std::endl;
        return;
    }

    if(m_lastActive < 0)
    {
        return;
    }

    while(--m_lastActive > 0)
    {
        int i = m_lastActive;
        Channel *ch = (Channel*)m_activeEvs[i].data.ptr;
        int revents = m_activeEvs[i].events;

        if(ch)
        {
            if(revent & (EPOLLERR | EPOLLHUP | EPOLLRDHUP))
            {
                revents |= ReadEvent | WriteEvent;   //在可读可写中进行处理
            }

            if(revents & ReadEvent)
            {
                ch->handleRead();
            }
            
            if(revents & WriteEvent)
            {
                ch->handleWrite();
            }
        }
    }
}




