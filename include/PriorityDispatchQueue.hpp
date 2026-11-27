// PriorityDispatchQueue.hpp - generic template priority queue (UNIT IV)
//
// Compare(a, b) must return true when `a` is LESS urgent than `b`
// (same convention as std::priority_queue): top() is then the most urgent item.
#pragma once

#include <algorithm>
#include <cstddef>
#include <functional>
#include <utility>
#include <vector>

#include "Exceptions.hpp"

namespace ReliefOps {
namespace Logistics {

template <typename T, typename Compare = std::less<T>>
class PriorityDispatchQueue {
public:
    explicit PriorityDispatchQueue(Compare cmp = Compare()) : cmp_(std::move(cmp)) {}

    void push(const T& value) {
        heap_.push_back(value);
        std::push_heap(heap_.begin(), heap_.end(), cmp_);
    }

    void push(T&& value) {
        heap_.push_back(std::move(value));
        std::push_heap(heap_.begin(), heap_.end(), cmp_);
    }

    const T& top() const {
        if (heap_.empty()) throw EmptyQueueException();
        return heap_.front();
    }

    T pop() {
        if (heap_.empty()) throw EmptyQueueException();
        std::pop_heap(heap_.begin(), heap_.end(), cmp_);
        T value = std::move(heap_.back());
        heap_.pop_back();
        return value;
    }

    bool empty() const noexcept { return heap_.empty(); }
    std::size_t size() const noexcept { return heap_.size(); }
    void clear() noexcept { heap_.clear(); }

    // Non-destructive view, most urgent first.
    std::vector<T> snapshot() const {
        std::vector<T> copy(heap_);
        std::sort(copy.begin(), copy.end(),
                  [this](const T& a, const T& b) { return cmp_(b, a); });
        return copy;
    }

private:
    std::vector<T> heap_;
    Compare cmp_;
};

}  // namespace Logistics
}  // namespace ReliefOps
