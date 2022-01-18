#include "tiny_sendthread.h"
#include <thread>

SendThread::SendThread():m_stop(false), m_msgEventQueueSize(0)
{
    sem_init(&m_semEventQueue, 0, 0);   //可能失败
}

SendThread::~SendThread()
{
    m_stop = true;
    sem_destroy(&m_semEventQueue);   //可能失败
}

std::shared_ptr<SendThread> SendThread::createThread()
{
    std::shared_ptr<SendThread> sp(new SendThread);
    std::weak_ptr<SendThread> wp(sp);
    std::thread th(SendThread::workThread, wp);
    return sp;
}

void SendThread::stopThread()
{
    m_stop = true;
}

bool SendThread::pushMsg(Msg&& msg)
{
    std::lock_guard<std::mutex> guard(m_mtxEventQueue);
    m_msgEventQueue.emplace_back(std::move(msg));
    ++m_msgEventQueueSize;
    sem_post(&m_semEventQueue);
    return true;
}

void SendThread::workThread(std::weak_ptr<SendThread> sendThread)
{
    while(true)
    {
        std::shared_ptr<SendThread> sp = sendThread.lock();

        if(!sp)
        {
            return;
        }

        if(sp->m_stop)
        {
            return;
        }

        //等待信号
        if(sem_wait(&sp->m_semEventQueue) == -1)
        {
            if(errno == EINTR)
            {
                continue;
            }
            //出错了
            else
            {
                sp->m_stop = true;  //停止吧
                return;
            }
        }

        if(sp->m_stop)
        {
            return;
        }

        if(sp->m_msgEventQueueSize > 0)
        {
            Msg msg;

            {
                std::lock_guard<std::mutex> guard(sp->m_mtxEventQueue);
                msg = std::move(sp->m_msgEventQueue.front());
                sp->m_msgEventQueue.pop_front();

            }
        }
    }
}