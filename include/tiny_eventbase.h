#ifndef __EVENT_BASE_H__
#define __EVENT_BASE_H__

#include "tiny_util.h"
#include <memory>

struct EventBases: private NonCopyable
{
    virtual EventBase *allocBase() = 0;
};

struct EventBase: private EventBases
{
    EventBase();
    ~EventBase();

    PollerBase* poller();          //底层poller
    void loopOnce(int waitMs);     //处理到期事件
    void loop();                   //事件循环
    void exit();                   //退出事件循环
    bool exited();                 //是否已退出
    void wakeUp();                 //管道唤醒
    virtual EventBase *allocBase() override { return this; }

private:
    struct EventBaseImpl;
    std::unique_ptr<EventBaseImpl> m_impl;
}; 

#endif