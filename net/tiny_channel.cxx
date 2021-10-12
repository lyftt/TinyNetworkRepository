#include "tiny_channel.h"
#include "tiny_poller.h"
#include "tiny_eventbase.h"
#include <unistd.h>

Channel::Channel(EventBase* base, int fd, int events):m_base(base), m_fd(fd), m_events(events)
{
    static std::atomic<int64_t> id(0);
    m_id = ++id;
    m_poller = m_base->poller();
    m_poller->addChannel(this);
}

Channel::~Channel()
{
    this->close();
}

void Channel::close()
{
    if(m_fd > 0)
    {
        m_poller->removeChannel(this);
        ::close(m_fd);
        m_fd = -1;
    }
}

void Channel::enableRead(bool enable)
{
    if(enable)
    {
        m_events |= ReadEvent;
    }else
    {
        m_events &= ~ReadEvent;
    }

    m_poller->updateChannel(this);
}

void Channel::enableWrite(bool enable)
{
    if(enable)
    {
        m_events |= WriteEvent;
    }else
    {
        m_events &= ~WriteEvent;
    }

    m_poller->updateChannel(this);
}

void Channel::enableReadWrite(bool readable, bool writable)
{
    if (readable) {
        m_events |= ReadEvent;
    } else {
        m_events &= ~ReadEvent;
    }
    if (writable) {
        m_events |= WriteEvent;
    } else {
        m_events &= ~WriteEvent;
    }
    m_poller->updateChannel(this);
}

bool Channel::readEnabled()
{
    return m_events & ReadEvent;
}

bool Channel::writeEnabled()
{
    return m_events & WriteEvent;
}