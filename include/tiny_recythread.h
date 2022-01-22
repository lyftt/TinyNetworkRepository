#ifndef __RECY_THREAD_H__
#define __RECY_THREAD_H__

#include <memory>
#include <atomic>
#include <mutex>
#include <semaphore.h>
#include <list>

class TcpConnection;

class RecyThread
{
public:
    ~RecyThread();

    //禁止拷贝和移动
    RecyThread(const RecyThread&) = delete;
    RecyThread& operator=(const RecyThread&) = delete;
    RecyThread(RecyThread&&) = delete;
    RecyThread& operator=(RecyThread&&) = delete;

    //停止发送线程
    void stopThread();
    //放入要回收的tcp连接
    bool pushConnection(TcpConnection* conn);

    //工厂函数，创建一个回收线程
    static std::shared_ptr<RecyThread> createThread();

private:
    //线程工作函数
    static void workThread(std::weak_ptr<RecyThread> recyThread);

private:
    RecyThread();

    //线程停止标志
    std::atomic<bool>          m_stop;
    std::atomic<size_t>        m_eventQueueSize;
    sem_t                      m_semEventQueue;
    std::mutex                 m_mtxEventQueue;
    std::list<TcpConnection*>  m_eventQueue;
};

#endif