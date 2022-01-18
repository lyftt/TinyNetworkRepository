#ifndef __SEND_THREAD_H__
#define __SEND_THREAD_H__

#include <memory>
#include <atomic>
#include <mutex>
#include <list>
#include <semaphore.h>
#include "tiny_codec.h"

struct SendThread: public std::enable_shared_from_this<SendThread>
{
    ~SendThread();

    //禁止拷贝和移动
    SendThread(const SendThread&) = delete;
    SendThread& operator=(const SendThread&) = delete;
    SendThread(SendThread&&) = delete;
    SendThread& operator=(SendThread&&) = delete;

    //停止发送线程
    void stopThread();
    //放入消息
    bool pushMsg(Msg&& msg);

    //工厂函数，创建一个发送线程
    static std::shared_ptr<SendThread> createThread();

private:
    //线程工作函数
    static void workThread(std::weak_ptr<SendThread> sendThread);

private:
    SendThread();

    //线程停止标志
    std::atomic<bool> m_stop;
    std::atomic<size_t> m_msgEventQueueSize;
    sem_t             m_semEventQueue;
    std::mutex        m_mtxEventQueue;
    std::list<Msg>    m_msgEventQueue;
};

#endif