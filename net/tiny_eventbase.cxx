#include "tiny_eventbase.h"
#include "tiny_poller.h"
#include "tiny_util.h"
#include "tiny_safequeue.h"
#include "tiny_channel.h"
#include <iostream>
#include <errno.h>
#include <cstring>
#include <unistd.h>
#include <atomic>
#include <map>
#include <cstdint>

struct TimerRepeatable {
    int64_t   m_at;
    int64_t   m_interval;
    TimerId   m_timerid;
    Task m_cb;
};

// 事件管理器实现类
struct EventBase::EventBaseImpl
{
    EventBase*   m_eventBase;      //关联的事件管理器对象
    PollerBase*  m_poller;         //事件管理器使用的poller
    int          m_wakeupFds[2];   //通知管道
    std::atomic<bool>  m_exit;     //退出事件循环标志,原子变量
    int          m_nextTimeOut;    //poller下次超时时间，用于实现定时器
    std::atomic<int64_t> m_timerSeq;  //定时器序号

    SafeQueue<Task> m_tasks;       //eventbase的任务队列

    std::map<TimerId, Task> m_timers;              //所有定时器map, 持久和非持久定时器都在这个map中，起到小顶堆的作用, 所有定时器进进出出
    std::map<TimerId, TimerRepeatable> m_repTimers; //持久定时器信息map, 到期之后的持久定时器会重新加入到m_timers中

    EventBaseImpl(EventBase* base):m_eventBase(base), m_poller(createPoller()), m_exit(false), m_nextTimeOut(1 << 30), m_timerSeq(0) {}
    ~EventBaseImpl(){
        delete m_poller;
    }

    void init();                //初始化
    void loop();                //进入事件循环
    void loopOnce(int waitMs);  //处理到期事件
    void wakeUp();              //通过管道唤醒
    void addTask(Task&& task);  //添加任务

    bool cancel(TimerId timerId);                                //取消定时器任务
    TimerId runAt(int64_t milli, Task &&task, int64_t interval); //创建定时器任务, interval不等于0则表示持久定时器

    void refreshNearestTimeout();   //刷新poller下次超时时间
    void handleTimeouts();          //处理到期定时器
    void repeatableTimeout(TimerRepeatable *tr);  //处理持久定时器

    void exit() { m_exit = true; wakeUp(); }  //终止事件循环
    bool exited() { return m_exit; }          //判断是否退出事件循环
    PollerBase* poller(){ return m_poller; }  //获取eventBase底层的poller
};

bool EventBase::EventBaseImpl::cancel(TimerId timerId)
{
    if(timerId.first < 0)
    {
        auto p = m_repTimers.find(timerId);
        auto pt = m_timers.find(p->second.m_timerid);
        if(pt != m_timers.end())
        {
            m_timers.erase(pt);
        }

        m_repTimers.erase(p);
        return true;
    }else{
        auto p = m_timers.find(timerId);
        if(p != m_timers.end())
        {
            m_timers.erase(p);
            return true;
        }

        return false;
    }
}

void EventBase::EventBaseImpl::addTask(Task&& task)
{
    m_tasks.push(std::move(task));
    wakeUp();
}

void EventBase::EventBaseImpl::repeatableTimeout(TimerRepeatable *tr)
{
    tr->m_at += tr->m_interval;    //刷新下次到期时间
    tr->m_timerid = {tr->m_at, ++m_timerSeq};
    m_timers[tr->m_timerid] = [this, tr] { repeatableTimeout(tr); };
    refreshNearestTimeout();
    tr->m_cb();
}

void EventBase::EventBaseImpl::handleTimeouts()
{
    int64_t now = util::timeMilli();
    TimerId tid{now, 1L << 62};

    //处理m_timers中所有到期的定时器
    while (m_timers.size() && m_timers.begin()->first < tid) {
        Task task = move(m_timers.begin()->second);
        m_timers.erase(m_timers.begin());
        task();
    }

    refreshNearestTimeout();   //刷新下次poller的超时时间
}

void EventBase::EventBaseImpl::refreshNearestTimeout()
{
    if(m_timers.empty())
        m_nextTimeOut = 1 << 30;
    else
    {
        const TimerId& tid = m_timers.begin()->first;
        m_nextTimeOut = tid.first - util::timeMilli();
        m_nextTimeOut = m_nextTimeOut < 0 ? 0 : m_nextTimeOut;
    }
}

TimerId EventBase::EventBaseImpl::runAt(int64_t milli, Task &&task, int64_t interval)
{
    if(m_exit)
        return TimerId();
    
    if(interval)
    {
        TimerId tid(-milli, ++m_timerSeq);   //持久定时器这里TimerId的first是负数，作为区分
        TimerRepeatable& rt = m_repTimers[tid];
        rt = {milli, interval, {milli, ++m_timerSeq}, move(task)};
        TimerRepeatable* prt = &rt;
        m_timers[prt->m_timerid] = [this, prt] { repeatableTimeout(prt); };
        refreshNearestTimeout();
        return tid;
    }else
    {
        TimerId tid(milli, ++m_timerSeq);
        m_timers.insert({tid, std::move(task)});
        refreshNearestTimeout();
        return tid;
    }
}

void EventBase::EventBaseImpl::init()
{
    int ret = pipe(m_wakeupFds);
    if(ret < 0)
    {
        std::cout<<"pipe failed, errno:"<<errno<<" "<<strerror(errno)<<std::endl;
        return;
    }

    Channel* ch = new Channel(m_eventBase, m_wakeupFds[0], ReadEvent);  //监听管道
    ch->onRead(
        [=]{
            char buf;
            int r = ch->id() >= 0 ? ::read(ch->fd(), &buf, 1) : 0;
            if(r > 0)
            {
                Task task;
                while(m_tasks.popWait(&task, 0))
                {
                    task();
                }
                std::cout<<"EventBaseImpl recv "<<buf<<" from pipe"<<std::endl;
            }
            else if(r == 0)
            {
                delete ch;
            }
            else if(errno == EINTR)
            {

            }
            else
            {
                std::cout<<"EventBaseImpl recv failed"<<std::endl;
            }
        }
    );
}

void EventBase::EventBaseImpl::wakeUp()
{
    int ret = ::write(m_wakeupFds[1], "c", 1);
    if(ret < 0)
    {
        std::cout<<"EventBaseImpl write failed,  errno:"<<errno<<" "<<strerror(errno)<<std::endl;
    }
}

void EventBase::EventBaseImpl::loop()
{
    while(!m_exit)
        loopOnce(10000);

    m_timers.clear();
    m_repTimers.clear();
    
    loopOnce(0);    //额外处理到期事件
}

void EventBase::EventBaseImpl::loopOnce(int waitMs)
{
    m_poller->loopOnce(std::min(waitMs, m_nextTimeOut));   //处理到期的读写事件
    handleTimeouts();             //处理到期的超时事件
}

EventBase::EventBase()
{
    m_impl.reset(new EventBaseImpl(this));
    m_impl->init();
}

EventBase::~EventBase()
{
    
}

PollerBase* EventBase::poller()
{
    return m_impl->poller();
}

void EventBase::loop()
{
    m_impl->loop();
}

void EventBase::loopOnce(int waitMs)
{
    m_impl->loopOnce(waitMs);
}

void EventBase::exit()
{
    m_impl->exit();
}

bool EventBase::exited()
{
    return m_impl->exited();
}

void EventBase::addTask(Task&& task)
{
    m_impl->addTask(std::move(task));
}

bool EventBase::cancel(TimerId timerId)
{
    return m_impl && m_impl->cancel(timerId);
}

TimerId EventBase::runAt(int64_t milli, Task &&task, int64_t interval)
{
    return m_impl->runAt(milli, std::move(task), interval);
}