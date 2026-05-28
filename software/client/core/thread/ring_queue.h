#ifndef RING_QUEUE_H
#define RING_QUEUE_H

#include <vector>
#include <mutex>
#include <condition_variable>

template<typename T>
class RingQueue {
public:
    explicit RingQueue(size_t capacity)
        : m_capacity(capacity + 1)
        , m_buffer(capacity + 1)
        , m_readPos(0)
        , m_writePos(0) {}

    // 阻塞式推入：如果队列满，则一直等到有空位为止
    void push(T&& item) {
        std::unique_lock<std::mutex> lock(m_mutex);

        // 当队列满时，阻塞等待
        m_notFullCV.wait(lock, [this]() {
            size_t next = (m_writePos + 1) % m_capacity;
            return next != m_readPos;
        });

        m_buffer[m_writePos] = std::move(item);
        m_writePos = (m_writePos + 1) % m_capacity;

        // 唤醒可能在等待读取的线程
        m_notEmptyCV.notify_one();
    }

    // 非阻塞式弹出（保留给不需要等待的场景）
    bool pop(T& item) {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (m_readPos == m_writePos) {
            return false;
        }

        item = std::move(m_buffer[m_readPos]);
        m_readPos = (m_readPos + 1) % m_capacity;

        m_notFullCV.notify_one();
        return true;
    }

    // 真正的阻塞式弹出：如果队列空，带超时的等待
    bool popWait(T& item, int timeoutMs = 10) {
        std::unique_lock<std::mutex> lock(m_mutex);

        // 等待队列不为空，最多等 timeoutMs 毫秒
        bool success = m_notEmptyCV.wait_for(lock, std::chrono::milliseconds(timeoutMs), [this]() {
            return m_readPos != m_writePos;
        });

        if (!success) {
            return false; // 超时了依然没数据
        }

        item = std::move(m_buffer[m_readPos]);
        m_readPos = (m_readPos + 1) % m_capacity;

        // 唤醒可能在等待写入的线程
        m_notFullCV.notify_one();
        return true;
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return (m_writePos >= m_readPos) ? (m_writePos - m_readPos) : (m_capacity - m_readPos + m_writePos);
    }

    void clear() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_readPos = 0;
        m_writePos = 0;
        m_notFullCV.notify_all();
    }

private:
    size_t m_capacity;
    std::vector<T> m_buffer;
    size_t m_readPos;
    size_t m_writePos;

    mutable std::mutex m_mutex;
    std::condition_variable m_notEmptyCV;
    std::condition_variable m_notFullCV;
};

#endif