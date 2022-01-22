#include "tiny_recythread.h"
#include <thread>
#include "tiny_tcpconnection.h"

RecyThread::RecyThread():m_stop(false), m_eventQueueSize(0)
{
    sem_init(&m_semEventQueue, 0, 0);
}

RecyThread::~RecyThread()
{
    m_stop = true;
    sem_destroy(&m_semEventQueue);
}

void RecyThread::stopThread()
{
    m_stop = true;
}

bool RecyThread::pushConnection(TcpConnection* conn)
{
    std::lock_guard<std::mutex> guard(m_mtxEventQueue);
    m_eventQueue.emplace_back(conn);
    ++m_eventQueueSize;
    sem_post(&m_semEventQueue);
    return true;
}

std::shared_ptr<RecyThread> RecyThread::createThread()
{
    std::shared_ptr<RecyThread> sp(new RecyThread);
    std::weak_ptr<RecyThread> wp(sp);
    std::thread th(RecyThread::workThread, wp);
    th.detach();
    return sp;
}

void RecyThread::workThread(std::weak_ptr<RecyThread> recyThread)
{
    while(true)
    {
        std::shared_ptr<RecyThread> sp = recyThread.lock();

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

        if(sp->m_eventQueueSize.load() > 0)
        {
            auto pos = sp->m_eventQueue.begin();
            auto posEnd = sp->m_eventQueue.end();
            TcpConnection* conn;

            {
                std::lock_guard<std::mutex> guard(sp->m_mtxEventQueue);   //锁住队列

                while(pos != posEnd)
                {
                    auto pos2 = pos;
                    
                    auto inTime = (*pos2)->m_inRecyTimePoint;
                    auto nowTime = std::chrono::steady_clock::now();
                    auto dura = std::chrono::duration_cast<std::chrono::seconds>(nowTime - inTime);

                    if (dura.count() < 10)  //10s回收时间
                    {
                        ++pos;
                        sem_post(&sp->m_semEventQueue);
                        continue;
                    }

                    ++pos;
                    (*pos2)->close();  //真正关闭连接
                    sp->m_eventQueue.erase(pos2);
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}