#include "tiny_signal.h"
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
    
    std::cin >> b;
    std::cout<< b << std::endl;

    return 0;
}