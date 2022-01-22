#include "tiny_sendthread.h"
#include "tiny_tcpconnection.h"
#include "tiny_codec.h"
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
    th.detach();
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

        if(sp->m_msgEventQueueSize.load() > 0)
        {
            auto pos = sp->m_msgEventQueue.begin();
            auto posEnd = sp->m_msgEventQueue.end();
            Msg msg;

            {
                std::lock_guard<std::mutex> guard(sp->m_mtxEventQueue);   //锁住队列
                CommonCodec codec;
                //msg = std::move(sp->m_msgEventQueue.front());
                //sp->m_msgEventQueue.pop_front();

                while(pos != posEnd)
                {
                    auto pos2 = pos;
                    //过期包
                    if(pos2->m_msgHead.m_tcpConnPtr->curSequence() != pos2->m_msgHead.m_curSeq)
                    {
                        ++pos;

                        sp->m_msgEventQueue.erase(pos2);
                        --sp->m_msgEventQueueSize;
                        continue;
                    }

                    //将要发送的报文进行编码
                    Buffer sendBuf = codec.encode(pos2->m_rpt); //编码成字节流
                    
                    //发送
                    auto ret = pos2->m_msgHead.m_tcpConnPtr->send(sendBuf);
                    //该连接已经启动了事件驱动，暂时不能发送，需要跳过这个msg
                    if(ret == -1)
                    {
                        ++pos;
                        sem_post(&sp->m_semEventQueue);  //因为这个跳过了，但是还留存在队列中，可能会滞留
                    }else
                    {
                        ++pos;

                        sp->m_msgEventQueue.erase(pos2);
                        --sp->m_msgEventQueueSize;
                        continue;
                    }
                }
            }
        }
    }
}