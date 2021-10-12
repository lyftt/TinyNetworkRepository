#include "tiny_util.h"
#include <chrono>

int64_t util::timeMicro()
{
    std::chrono::time_point<std::chrono::steady_clock> p = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(p.time_since_epoch()).count();
}