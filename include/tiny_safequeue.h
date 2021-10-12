#ifndef __SAFEQUEUE_H__
#define __SAFEQUEUE_H__

#include <list>
#include <mutex>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <functional>
#include <limits>
#include "tiny_util.h"

template <typename T>
struct SafeQueue: private std::mutex, private NonCopyable
{
    static const int wait_infinite = std::numeric_limits<int>::max();   //int型别最大值

    SafeQueue(ssize_t capacity=0):m_capacity(capacity), m_exit(false){}

    bool push(T&& t);
    T popWait(int waitMs=wait_infinite);
    bool popWait(T* t, int waitMs=wait_infinite);

    ssize_t capacity();
    ssize_t size();
    void exit();
    bool exited() { return m_exit; }

private:
    std::list<T>            m_items;     //内部队列
    std::condition_variable m_cond;      //信号量
    size_t                  m_capacity;  //当前容量
    std::atomic<bool>       m_exit;      //退出标志

    void waitReady(std::unique_lock<std::mutex>& lk, int waitMs);
};

template <typename T>
bool SafeQueue<T>::push(T&& t)   //万能引用,左值右值都可以适配
{
    std::lock_guard<std::mutex> guard(*this);

    if(m_exit || m_capacity && m_items.size() >= m_capacity)
        return false;
    
    m_items.push_back(std::move(t));
    m_cond.notify_one();
    return true;
}

template <typename T>
T SafeQueue<T>::popWait(int waitMs)
{
    std::unique_lock<std::mutex> lk(*this);
    waitReady(lk, waitMs);

    if(m_items.empty())
        return T();
    
    T r = std::move(m_items.front());
    m_items.pop_front();
    return r;
}

template <typename T>
bool SafeQueue<T>::popWait(T* t, int waitMs)
{
    std::unique_lock<std::mutex> lk(*this);
    waitReady(lk, waitMs);

    if(m_items.empty())
        return false;
    
    *t = std::move(m_items.front());
    m_items.pop_front();
    return true;
}

template <typename T>
void SafeQueue<T>::waitReady(std::unique_lock<std::mutex>& lk, int waitMs)
{
    if(m_exit || !m_items.empty())
        return;
    
    if(waitMs == wait_infinite)
    {
        m_cond.wait(lk, [this] { return m_exit || !m_items.empty(); });
    }else if(waitMs > 0)
    {
        auto tp = std::chrono::steady_clock::now() + std::chrono::milliseconds(waitMs);
        //m_cond.wait_until(lk, tp, [this] { !m_items.empty() || m_exit});
        while(m_cond.wait_until(lk, tp) != std::cv_status::timeout && m_items.empty() && !m_exit){

        }
    }
}

template <typename T>
ssize_t SafeQueue<T>::size()
{
    std::lock_guard<std::mutex> guard(*this);
    return m_items.size();
}

template <typename T>
ssize_t SafeQueue<T>::capacity()
{
    std::lock_guard<std::mutex> guard(*this);
    return m_capacity;
}

template <typename T>
void SafeQueue<T>::exit()
{
    m_exit = true;     //这里不需要先加锁
    std::lock_guard<std::mutex> guard(*this);
    m_cond.notify_all();
}

#endif