#ifndef __SIGNAL_H__
#define __SIGNAL_H__

#include <signal.h>
#include <functional>

struct SignalRegister
{
    int          sigNo;
    const char * sigName;
    std::function<void()> sigHandler;
};

struct Signal
{
    enum class SignalLevel
    {
        LOW_SIGNAL,
        HIGH_SIGNAL
    };

    static void signalRegiste(SignalLevel level, SignalRegister* regs, unsigned int regNum);
};

#endif