#include "tiny_signal.h"
#include <map>
#include <iostream>
#include <cstring>

std::map<int, std::function<void()>> signalHandlerMap;

// 统一的信号处理函数入口
static void signalUniformHandlerHigh(int sigNo, siginfo_t *sigInfo, void *uContext)
{
    if(signalHandlerMap.find(sigNo) == signalHandlerMap.end()) return;

    signalHandlerMap[sigNo]();

    // 可以从siginfo_t 结构中获得发送信号的进程pid
    if(sigInfo && sigInfo->si_pid)
    {
        std::cout << "get signal from process:" << sigInfo->si_pid << std::endl;
    }
}

static void signalUniformHandlerLow(int sigNo)
{
    if(signalHandlerMap.find(sigNo) == signalHandlerMap.end()) return;

    signalHandlerMap[sigNo]();
}

void Signal::signalRegiste(SignalLevel level, SignalRegister* regs, unsigned int regNum)
{
    struct sigaction  sa;

    for(int i = 0; i < regNum; ++i)
    {
         memset(&sa, 0, sizeof(struct sigaction));

        if(regs[i].sigNo == 0) continue;

        signalHandlerMap[regs[i].sigNo] = regs[i].sigHandler;

        if(level == SignalLevel::LOW_SIGNAL)
        {
            signal(regs[i].sigNo, signalUniformHandlerLow);
        }
        else if(level == SignalLevel::HIGH_SIGNAL)
        {
            sa.sa_sigaction = signalUniformHandlerHigh;
            sa.sa_flags = SA_SIGINFO | SA_RESTART;
            sigemptyset(&sa.sa_mask); 
            sigaction(regs[i].sigNo, &sa, NULL);
        }
    }
}