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
                //msg = std::move(sp->m_msgEventQueue.front());
                //sp->m_msgEventQueue.pop_front();

                while(pos != posEnd)
                {
                    //过期包
                    if(pos->m_msgHead.m_tcpConnPtr->m_curSequence.load() != pos->m_msgHead.m_curSeq)
                    {
                        auto pos2 = pos;
                        ++pos;

                        sp->m_msgEventQueue.erase(pos2);
                        --sp->m_msgEventQueueSize;
                        continue;
                    }

                    //如果该条连接现在在使用驱动发送，则说明发送区比较满，跳过
                    if(pos->m_msgHead.m_tcpConnPtr->m_useSendEvent.load())
                    {
                        ++pos;
                        continue;
                    }

                    //将要发送的报文进行编码
                    auto pos2 = pos;
                    CommonCodec codec;
                    Buffer sendBuf = codec.encode(pos2->m_rpt);
                    //开始直接发送数据
                    int ret = ::send(pos2->m_msgHead.m_tcpConnPtr->m_fd, sendBuf.data(), sendBuf.size(), 0);

                    if(ret > 0)
                    {
                        //全部发送完
                        if(ret == sendBuf.size())
                        {
                            ++pos;
                            sp->m_msgEventQueue.erase(pos2);
                            continue;
                        }
                        //没有全部发送完，说明发送缓冲满了，使用驱动来发送这个报文剩下的部分s
                        else
                        {
                            pos2->m_msgHead.m_tcpConnPtr->m_useSendEvent = true; //开始使用驱动发送
                            sendBuf.consumed(ret);
                            pos2->m_msgHead.m_tcpConnPtr->m_writeBuffer.append(sendBuf.data(), sendBuf.size());  //添加到tcp连接的发送缓冲区
                        }
                    }
                    else
                    {

                    }
                }
            }
        }
    }
}