#ifndef __EVENT_BASE_H__
#define __EVENT_BASE_H__

#include "tiny_util.h"
#include <memory>
#include <functional>
struct PollerBase;
struct EventBase;

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

    bool cancel(TimerId timerId);  //取消定时任务
    //定时器任务,interval=0表示非持久定时器,milli和interval的单位都是毫秒
    TimerId runAt(int64_t milli, const Task &task, int64_t interval = 0) { return runAt(milli, Task(task), interval); } 
    TimerId runAt(int64_t milli, Task &&task, int64_t interval = 0);
    TimerId runAfter(int64_t milli, const Task &task, int64_t interval = 0) { return runAt(util::timeMilli() + milli, Task(task), interval); }
    TimerId runAfter(int64_t milli, Task &&task, int64_t interval = 0) { return runAt(util::timeMilli() + milli, std::move(task), interval); }

    void addTask(Task&& task);     //向eventbase的任务队列添加任务
    void addTask(const Task& task){
        addTask(Task(task));
    }

    virtual EventBase *allocBase() override { return this; }

private:
    struct EventBaseImpl;
    std::unique_ptr<EventBaseImpl> m_impl;
}; 

#endif