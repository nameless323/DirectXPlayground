#pragma once

#include <queue>
#include <optional>
#include <mutex>

template <typename T>
class ThreadSafeQueue
{
public:
    ThreadSafeQueue() = default;
    ThreadSafeQueue(ThreadSafeQueue&& other);
    ThreadSafeQueue(const ThreadSafeQueue<T>&) = delete;
    ThreadSafeQueue& operator= (const ThreadSafeQueue<T>&) = delete;

    virtual ~ThreadSafeQueue() = default;

    size_t Size() const;
    std::optional<T> Pop();
    void Push(const T& item);
    void Push(T&& item);
    std::optional<T> Front() const;

private:
    std::queue<T> mQueue;
    mutable std::mutex mMutex;
};

template <typename T>
inline ThreadSafeQueue<T>::ThreadSafeQueue(ThreadSafeQueue<T>&& other)
{
    std::scoped_lock l(mMutex);
    std::swap(mQueue, other.mQueue);
}

template <typename T>
size_t ThreadSafeQueue<T>::Size() const
{
    std::scoped_lock l(mMutex);
    return mQueue.size();
}

template <typename T>
std::optional<T> ThreadSafeQueue<T>::Pop()
{
    std::scoped_lock l(mMutex);
    if (mQueue.empty())
        return {};
    T tmp = mQueue.front();
    mQueue.pop();
    return tmp;
}

template <typename T>
void ThreadSafeQueue<T>::Push(const T& item)
{
    std::scoped_lock l(mMutex);
    mQueue.push(item);
}

template <typename T>
void ThreadSafeQueue<T>::Push(T&& item)
{
    std::scoped_lock l(mMutex);
    mQueue.push(std::forward<T>(item));
}

template <typename T>
std::optional<T> ThreadSafeQueue<T>::Front() const
{
    std::scoped_lock l(mMutex);
    if (mQueue.empty())
        return {};
    return mQueue.front();
}