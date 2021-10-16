#ifndef __UTIL_H__
#define __UTIL_H__

#include <cstdlib>
#include <cstdint>
#include <functional>

using Task = std::function<void()>;
using TimerId = std::pair<int64_t, int64_t>;
//typedef std::function<void()> Task;

struct NonCopyable
{
    NonCopyable() = default;
    virtual ~NonCopyable() = default;

    NonCopyable(const NonCopyable&) = delete;         //禁止拷贝构造
    NonCopyable& operator=(const NonCopyable&) = delete; //禁止拷贝赋值
};

struct util
{
    static int64_t timeMicro();          //当前时间（微秒）
    static int64_t timeMilli() { return timeMicro() / 1000; }   //当前时间（毫秒）
};

#endif