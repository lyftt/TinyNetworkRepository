#include "tiny_signal.h"
#include "tiny_eventbase.h"
#include "tiny_tcpserver.h"
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

    TcpServer svr(base, "127.0.0.1", 6543);
    svr.OnRead([](TcpConnection&){
        std::cout<<"in tcp read task"<<std::endl;
    });

    base->loop();

    return 0;
}