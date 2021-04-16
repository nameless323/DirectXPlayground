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
    std::queue<T> m_queue;
    mutable std::mutex m_mutex;
};

template <typename T>
inline ThreadSafeQueue<T>::ThreadSafeQueue(ThreadSafeQueue<T>&& other)
{
    std::scoped_lock l(m_mutex);
    std::swap(m_queue, other.m_queue);
}

template <typename T>
size_t ThreadSafeQueue<T>::Size() const
{
    std::scoped_lock l(m_mutex);
    return m_queue.size();
}

template <typename T>
std::optional<T> ThreadSafeQueue<T>::Pop()
{
    std::scoped_lock l(m_mutex);
    if (m_queue.empty())
        return {};
    T tmp = m_queue.front();
    m_queue.pop();
    return tmp;
}

template <typename T>
void ThreadSafeQueue<T>::Push(const T& item)
{
    std::scoped_lock l(m_mutex);
    m_queue.push(item);
}

template <typename T>
void ThreadSafeQueue<T>::Push(T&& item)
{
    std::scoped_lock l(m_mutex);
    m_queue.push(std::forward(item));
}

template <typename T>
std::optional<T> ThreadSafeQueue<T>::Front() const
{
    std::scoped_lock l(m_mutex);
    if (m_queue.empty())
        return {};
    return m_queue.front();
}