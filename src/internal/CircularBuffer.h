#pragma once

#include <cstddef>
#include <atomic>

namespace zlnetpp {
namespace internal {

class CircularBuffer {
public:
    explicit CircularBuffer(unsigned int order);
    ~CircularBuffer();

    bool isValid() const { return data_ != nullptr; }
    size_t size() const { return size_; }
    
    // Raw pointer access
    unsigned char* headPtr() const;
    unsigned char* tailPtr() const;

    // Atomic access for MpscQueue
    std::atomic<unsigned int>& headAtomic() { return head_; }
    std::atomic<unsigned int>& tailAtomic() { return tail_; }

    unsigned char* getPointer(unsigned int offset) const;

private:
    void createBufferMirror();

    size_t size_;
    unsigned char* data_;
    std::atomic<unsigned int> head_;
    std::atomic<unsigned int> tail_;
};

} // namespace internal
} // namespace zlnetpp
