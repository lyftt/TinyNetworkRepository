#include "tiny_signal.h"
#include "tiny_eventbase.h"
#include "tiny_tcpserver.h"
#include "tiny_tcpconnection.h"
#include "tiny_codec.h"
#include "tiny_threadpool.h"
#include "tiny_sendthread.h"
#include <iostream>
#include <cstring>

static void signalUniformHandler(int sigNo)
{
    std::cout<<"asd"<<std::endl;
}

//创建业务线程池
auto g_workPool = ThreadPool::Create(5, 10, std::chrono::seconds(1), 10000);

//创建发送线程
auto g_sendThread = SendThread::createThread();

int main()
{
    char b = 'a';
    SignalRegister reg[2] = {{SIGINT, "SIGINT", [&]{ std::cout<<"process  SIGINT " << b <<std::endl; }}, {SIGUSR1, "SIGUSR1", [&] { std::cout<<"process  SIGUSR1" << b <<std::endl; }}};
    Signal::signalRegiste(Signal::SignalLevel::HIGH_SIGNAL, reg, 2);
    
    EventBase* base = new EventBase;

    //一些定时任务
    //base->runAfter(1000, []{ std::cout<<"timer reach"<<std::endl; }, 1000);
    //base->addTask([]{ std::cout<<"task process1"<<std::endl; });
    //base->addTask([]{ std::cout<<"task process2"<<std::endl; });

    //业务线程池启动
    g_workPool->start();

    //将tcp svr 挂到eventbase上
    TcpServer svr(base, "127.0.0.1", 6543);
    svr.OnRead([](TcpConnection& c){
        std::cout<<"in tcp read task, now content in recv bufer:"<< c.getRecvBufferAddr() <<std::endl;

        CommonCodec codec;

        //缓冲区可能跟不止一个报文
        while(1)
        {
            std::tuple<long, Rpt> reportTup = codec.decode(c.getRecvBufferAddr(), c.getRecvBufferSize());

            //缓冲区有问题
            if(std::get<0>(reportTup) < 0)
            {
                std::cout<<"tcp recv buffer is not right now, try to close"<<std::endl;
                c.close();
                break;
            }
            //缓冲区数据不够一个报文
            else if(std::get<0>(reportTup) == 0)
            {
                std::cout<<"tcp recv buffer has data, but not up to one report"<<std::endl;
                break;
            }

            //从缓冲区中解出一个报文
            Rpt report = std::move(std::get<1>(reportTup));
            std::cout<<"decode one report, len:"<< report.m_rptHead.m_rptLen << " type:" << report.m_rptHead.m_type<< " content:" << report.m_rptBuffer.data() << std::endl;
            c.consumedRecvBufferSize(std::get<0>(reportTup));  //从缓冲区中移除消费掉的数据

            //将报文组成一个消息
            Msg msg;
            msg.m_msgHead.m_curSeq = c.m_curSequence.load();
            msg.m_msgHead.m_tcpConnPtr = &c;
            msg.m_rpt = std::move(report);

            //将消息放入线程池中处理
            //要将对象移入lambda表达式，c++11中只能这么做; c++14可以使用广义捕获，非常简单
            auto result = g_workPool->enqueue(std::bind([] (const Msg& msg)
                {
                    Msg resultMsg;
                    resultMsg.m_msgHead = msg.m_msgHead;
                    resultMsg.m_rpt.m_rptHead.m_type = 1;
                    resultMsg.m_rpt.m_rptHead.m_rptLen = 8 + strlen("recv ok, notify you");
                    resultMsg.m_rpt.m_rptBuffer.append("recv ok, notify you", strlen("recv ok, notify you"));

                    g_sendThread->pushMsg(std::move(resultMsg));
                    //return std::move(resultMsg);
                },
                std::move(msg)  //会被移动到bind返回的绑定对象中
                )
            );

            if(result == nullptr)
            {
                std::cout<<"thread enqueue failed, return nullptr"<<std::endl;
                continue;
            }

            //将期望送入发送线程
            //Msg m = std::move(result->get());
            //std::cout<<m.m_rpt.m_rptBuffer.data()<<std::endl; 
        }
        
    });

    base->loop();

    return 0;
}