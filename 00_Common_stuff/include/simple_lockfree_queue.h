#ifndef _SIMPLE_LOCKFREE_QUEUE_H_
#define _SIMPLE_LOCKFREE_QUEUE_H_

#include <atomic>
#include <memory>

template<typename T>
class SimpleLockFreeQueue {
private:
    struct Node {
        std::unique_ptr<T> data;
        std::atomic<Node*> next;
        
        Node() : data(nullptr), next(nullptr) {}
        Node(T&& value) : data(std::make_unique<T>(std::move(value))), next(nullptr) {}
    };
    
    std::atomic<Node*> head;
    std::atomic<Node*> tail;

public:
    SimpleLockFreeQueue() {
        Node* dummy = new Node();
        head.store(dummy, std::memory_order_relaxed);
        tail.store(dummy, std::memory_order_relaxed);
    }
    
    ~SimpleLockFreeQueue() {
        while (pop()) {}
        delete head.load();
    }
    
    SimpleLockFreeQueue(const SimpleLockFreeQueue&) = delete;
    SimpleLockFreeQueue& operator=(const SimpleLockFreeQueue&) = delete;
    
    void push(T value) {
        Node* new_node = new Node(std::move(value));
        
        while (true) {
            Node* current_tail = tail.load(std::memory_order_acquire);
            Node* next = current_tail->next.load(std::memory_order_acquire);
            
            if (current_tail == tail.load(std::memory_order_acquire)) {
                if (next == nullptr) {
                    if (current_tail->next.compare_exchange_weak(
                        next, new_node, std::memory_order_release)) {
                        tail.compare_exchange_weak(
                            current_tail, new_node, std::memory_order_release);
                        return;
                    }
                } else {
                    tail.compare_exchange_weak(
                        current_tail, next, std::memory_order_release);
                }
            }
        }
    }
    
    std::unique_ptr<T> pop() {
        while (true) {
            Node* current_head = head.load(std::memory_order_acquire);
            Node* current_tail = tail.load(std::memory_order_acquire);
            Node* next = current_head->next.load(std::memory_order_acquire);
            
            if (current_head == head.load(std::memory_order_acquire)) {
                if (current_head == current_tail) {
                    if (next == nullptr) {
                        return nullptr;
                    }
                    tail.compare_exchange_weak(
                        current_tail, next, std::memory_order_release);
                } else {
                    if (head.compare_exchange_weak(
                        current_head, next, std::memory_order_release)) {
                        std::unique_ptr<T> result = std::move(next->data);
                        delete current_head;
                        return result;
                    }
                }
            }
        }
    }
    
    bool empty() const {
        Node* h = head.load(std::memory_order_acquire);
        Node* t = tail.load(std::memory_order_acquire);
        Node* n = h->next.load(std::memory_order_acquire);
        return h == t || n == nullptr;
    }
};

#endif /* _SIMPLE_LOCKFREE_QUEUE_H_ */