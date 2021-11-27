#ifndef __THREADPOOL_H__
#define __THREADPOOL_H__

#include <memory>

struct ThreadPool
{
    ThreadPool();
    ~ThreadPool();

private:
    struct  ThreadImpl;
    std::unique_ptr<ThreadImpl>  m_impl;
};

#endif