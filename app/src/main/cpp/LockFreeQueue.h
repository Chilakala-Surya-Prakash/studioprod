#ifndef LOCK_FREE_QUEUE_H
#define LOCK_FREE_QUEUE_H

#include <atomic>
#include <vector>
#include <cstddef>
#include <algorithm>

// Single-Producer Single-Consumer (SPSC) Lock-Free Circular Ring Buffer
// Designed for real-time audio threads to prevent priority inversions,
// heap allocations, and mutex contention.
template <typename T, size_t Capacity = 65536>
class SpscRingBuffer {
public:
    SpscRingBuffer() : mHead(0), mTail(0) {
        mBuffer.resize(Capacity);
    }

    void reset() {
        mHead.store(0, std::memory_order_relaxed);
        mTail.store(0, std::memory_order_relaxed);
    }

    size_t getCapacity() const { return Capacity - 1; }

    size_t availableRead() const {
        size_t head = mHead.load(std::memory_order_acquire);
        size_t tail = mTail.load(std::memory_order_relaxed);
        if (head >= tail) {
            return head - tail;
        }
        return Capacity + head - tail;
    }

    size_t availableWrite() const {
        return getCapacity() - availableRead();
    }

    size_t write(const T* data, size_t count) {
        size_t head = mHead.load(std::memory_order_relaxed);
        size_t tail = mTail.load(std::memory_order_acquire);

        size_t available;
        if (head >= tail) {
            available = Capacity - (head - tail) - 1;
        } else {
            available = tail - head - 1;
        }

        size_t toWrite = std::min(count, available);
        if (toWrite == 0) return 0;

        size_t firstChunk = std::min(toWrite, Capacity - head);
        std::copy(data, data + firstChunk, mBuffer.data() + head);

        size_t secondChunk = toWrite - firstChunk;
        if (secondChunk > 0) {
            std::copy(data + firstChunk, data + toWrite, mBuffer.data());
        }

        mHead.store((head + toWrite) % Capacity, std::memory_order_release);
        return toWrite;
    }

    size_t read(T* data, size_t count) {
        size_t head = mHead.load(std::memory_order_acquire);
        size_t tail = mTail.load(std::memory_order_relaxed);

        size_t available;
        if (head >= tail) {
            available = head - tail;
        } else {
            available = Capacity + head - tail;
        }

        size_t toRead = std::min(count, available);
        if (toRead == 0) return 0;

        size_t firstChunk = std::min(toRead, Capacity - tail);
        std::copy(mBuffer.data() + tail, mBuffer.data() + tail + firstChunk, data);

        size_t secondChunk = toRead - firstChunk;
        if (secondChunk > 0) {
            std::copy(mBuffer.data(), mBuffer.data() + secondChunk, data + firstChunk);
        }

        mTail.store((tail + toRead) % Capacity, std::memory_order_release);
        return toRead;
    }

private:
    std::vector<T> mBuffer;
    alignas(64) std::atomic<size_t> mHead;
    alignas(64) std::atomic<size_t> mTail;
};

#endif // LOCK_FREE_QUEUE_H
