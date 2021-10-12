#ifndef __CHANNEL_H__
#define __CHANNEL_H__

#include "tiny_util.h"
#include <functional>
struct EventBase;
struct PollerBase;

//通道封装一个fd，不携带应用层缓冲区
struct Channel: private NonCopyable
{
    //fd是封装的文件描述符, events是要监听的事件
    Channel(EventBase* base, int fd, int events);
    virtual ~Channel();

    int id(){ return m_id; }
    int fd(){ return m_fd; }
    short events() { return m_events; }

    //关闭通道
    void close();                     

    //挂接事件处理器
    void onRead(const Task &readcb) { m_readCb = readcb; }
    void onWrite(const Task &writecb) { m_writeCb = writecb; }
    void onRead(Task &&readcb) { m_readCb = std::move(readcb); }
    void onWrite(Task &&writecb) { m_writeCb = std::move(writecb); }

    //启用读写监听
    void enableRead(bool enable);
    void enableWrite(bool enable);
    void enableReadWrite(bool readable, bool writable);
    bool readEnabled();
    bool writeEnabled();

    //处理读写事件
    void handleRead() { m_readCb(); }
    void handleWrite() { m_writeCb(); }

    int         m_id;                 //通道id
    int         m_fd;                 //通道包装的文件描述符
    PollerBase* m_poller;             //关联的poller
    EventBase*  m_base;               //关联的事件管理器
    short       m_events;             //该通道要监听的事件
    std::function<void()> m_readCb;
    std::function<void()> m_writeCb;
    std::function<void()> m_errorCb;
};

#endif