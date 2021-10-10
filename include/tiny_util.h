#ifndef __UTIL_H__
#define __UTIL_H__

#include <cstdlib>
#include <cstdint>

using Task = std::function<void()>;
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
    static int64_t timeMicro();
    static int64_t timeMilli() { return timeMicro() / 1000; }
};

#endif