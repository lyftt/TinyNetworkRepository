#include "tiny_util.h"
#include <chrono>

int64_t util::timeMicro()
{
    chrono::time_point<chrono::steady_clock> p = chrono::steady_clock::now();
    return chrono::duration_cast<chrono::microseconds>(p.time_since_epoch()).count();
}