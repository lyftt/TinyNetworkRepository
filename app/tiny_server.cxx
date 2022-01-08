#include "tiny_signal.h"
#include "tiny_eventbase.h"
#include "tiny_tcpserver.h"
#include "tiny_tcpconnection.h"
#include "tiny_codec.h"
#include <iostream>
#include <cstring>

static void signalUniformHandler(int sigNo)
{
    std::cout<<"asd"<<std::endl;
}

int main()
{
    char b = 'a';
    SignalRegister reg[2] = {{SIGINT, "SIGINT", [&]{ std::cout<<"process  SIGINT " << b <<std::endl; }}, {SIGUSR1, "SIGUSR1", [&] { std::cout<<"process  SIGUSR1" << b <<std::endl; }}};
    Signal::signalRegiste(Signal::SignalLevel::HIGH_SIGNAL, reg, 2);
    
    EventBase* base = new EventBase;

    //base->runAfter(1000, []{ std::cout<<"timer reach"<<std::endl; }, 1000);
    //base->addTask([]{ std::cout<<"task process1"<<std::endl; });
    //base->addTask([]{ std::cout<<"task process2"<<std::endl; });

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
                std::cout<<"tcp recv buffer has data, but not up to one report, try to close"<<std::endl;
                break;
            }

            //从缓冲区中解出一个报文
            Rpt report = std::move(std::get<1>(reportTup));
            std::cout<<"decode one report, len:"<< report.m_rptHead.m_rptLen << " type:" << report.m_rptHead.m_type<< " content:" << report.m_rptBuffer.data() << std::endl;
            c.consumedRecvBufferSize(std::get<0>(reportTup));  //从缓冲区中移除消费掉的数据

            //放入线程池中处理
        }
        
    });

    base->loop();

    return 0;
}