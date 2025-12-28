#pragma once

#include "../../src/internal/CircularBuffer.h"
#include <atomic>
#include <memory>
#include <type_traits>
#include <cstring>

namespace hvnetpp {

// T must be trivially copyable to be stored in the raw buffer safely
template <typename T>
class MpscQueue {
public:
    struct Node {
        T data;
        std::atomic<uint32_t> id;
        // Pad to 64 bytes (cache line size) to avoid false sharing and align access
        char padding[64 - sizeof(std::atomic<uint32_t>) - sizeof(T)];
    };
    
    static_assert(sizeof(Node) == 64, "Node size must be 64 bytes");

    explicit MpscQueue(unsigned int size_order = 16)
        : buffer_(new internal::CircularBuffer(size_order)) {
    }

    ~MpscQueue() {
    }

    bool isValid() const { return buffer_->isValid(); }

    // Reserve a slot for writing
    Node* reserve() {
        const uint32_t nodeSize = sizeof(Node);
        unsigned int s = buffer_->size();
        
        while (true) {
            unsigned int r = buffer_->headAtomic().load(std::memory_order_relaxed);
            unsigned int w = buffer_->tailAtomic().load(std::memory_order_relaxed);

            // Check overflow
            if ((w - r) > (s - nodeSize)) {
                return nullptr;
            }

            if (buffer_->tailAtomic().compare_exchange_weak(w, w + nodeSize)) {
                return reinterpret_cast<Node*>(buffer_->getPointer(w));
            }
        }
    }

    void commit(Node* node, uint32_t id) {
        std::atomic_thread_fence(std::memory_order_release);
        node->id.store(id, std::memory_order_release);
    }

    Node* peek() {
        if (buffer_->headAtomic().load(std::memory_order_relaxed) == buffer_->tailAtomic().load(std::memory_order_acquire)) {
            return nullptr;
        }
        
        Node* node = reinterpret_cast<Node*>(buffer_->headPtr());
        if (node->id.load(std::memory_order_acquire)) {
            return node;
        }
        return nullptr;
    }

    void consume(Node* node) {
        memset(node, 0, sizeof(Node));
        std::atomic_thread_fence(std::memory_order_release);
        buffer_->headAtomic().fetch_add(sizeof(Node), std::memory_order_relaxed);
    }

private:
    std::unique_ptr<internal::CircularBuffer> buffer_;
};

} // namespace hvnetpp