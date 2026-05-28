#ifndef THREAD_SAFE_QUEUE_H
#define THREAD_SAFE_QUEUE_H

#include <atomic>
#include <vector>
#include <thread>
#include <chrono>

/**
 * 无锁环形队列（单生产者单消费者）
 * 接口与 ThreadSafeQueue 兼容，无需修改调用代码
 */
template<typename T>
class ThreadSafeQueue {
public:
    explicit ThreadSafeQueue(size_t maxSize = 100)
        : m_capacity(maxSize + 1), m_buffer(maxSize + 1), m_readPos(0), m_writePos(0) {}

    // 禁止拷贝
    ThreadSafeQueue(const ThreadSafeQueue&) = delete;
    ThreadSafeQueue& operator=(const ThreadSafeQueue&) = delete;

    // 入队（移动语义）
    bool push(T&& item, int timeoutMs = 0) {
        size_t write = m_writePos.load(std::memory_order_relaxed);
        size_t next = (write + 1) % m_capacity;

        // 队列满时等待
        while (next == m_readPos.load(std::memory_order_acquire)) {
            if (timeoutMs > 0) {
                return false;
            }
            std::this_thread::yield();
            write = m_writePos.load(std::memory_order_relaxed);
            next = (write + 1) % m_capacity;
        }

        m_buffer[write] = std::move(item);
        m_writePos.store(next, std::memory_order_release);
        return true;
    }

    // 出队
    bool pop(T& item, int timeoutMs = 0) {
        size_t read = m_readPos.load(std::memory_order_relaxed);

        while (read == m_writePos.load(std::memory_order_acquire)) {
            if (timeoutMs > 0) {
                return false;
            }
            std::this_thread::yield();
            read = m_readPos.load(std::memory_order_relaxed);
        }

        item = std::move(m_buffer[read]);
        m_readPos.store((read + 1) % m_capacity, std::memory_order_release);
        return true;
    }

    // 非阻塞出队
    bool tryPop(T& item) {
        size_t read = m_readPos.load(std::memory_order_relaxed);

        if (read == m_writePos.load(std::memory_order_acquire)) {
            return false;
        }

        item = std::move(m_buffer[read]);
        m_readPos.store((read + 1) % m_capacity, std::memory_order_release);
        return true;
    }

    // 清空队列
    void clear() {
        T dummy;
        while (tryPop(dummy));
    }

    // 获取大小（近似值）
    size_t size() const {
        size_t write = m_writePos.load(std::memory_order_acquire);
        size_t read = m_readPos.load(std::memory_order_acquire);
        if (write >= read) {
            return write - read;
        } else {
            return m_capacity - read + write;
        }
    }

    bool empty() const {
        return m_readPos.load(std::memory_order_acquire) ==
               m_writePos.load(std::memory_order_acquire);
    }

    bool full() const {
        size_t write = m_writePos.load(std::memory_order_acquire);
        size_t next = (write + 1) % m_capacity;
        return next == m_readPos.load(std::memory_order_acquire);
    }

private:
    size_t m_capacity;
    std::vector<T> m_buffer;
    std::atomic<size_t> m_readPos;
    std::atomic<size_t> m_writePos;
};

#endif