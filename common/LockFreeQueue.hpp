#pragma once
#include <atomic>
#include <iostream>
#include <vector>

#include "common/Macros.hpp"

namespace Common {
template<typename T>
class LockFreeQueue final {
public:
  LockFreeQueue(std::size_t num_elements_) : store(num_elements_, T()) {}

  LockFreeQueue() = delete;
  LockFreeQueue(const LockFreeQueue&) = delete;
  LockFreeQueue(const LockFreeQueue&&) = delete;
  LockFreeQueue& operator=(const LockFreeQueue&) = delete;
  LockFreeQueue& operator=(const LockFreeQueue&&) = delete;

  auto getNextToWrite() noexcept {
    return &store[next_write_index];  // return the starting memory address of
                                      // the next writable element
  }
  auto updateWriteIndex() noexcept {
    next_write_index = (next_write_index + 1) % store.size();
    num_elements++;
  }
  auto getNextToRead() const noexcept -> const T* {
    return (next_read_index == next_write_index ? nullptr
                                                : &store[next_read_index]);
  }

  auto updateReadIndex() noexcept {
    next_read_index = (next_read_index + 1) % store.size();
    ASSERT(num_elements != 0,
           "Read an invalid element in : " + std::to_string(pthread_self()));
    num_elements--;
  }

  auto size() const noexcept { return num_elements.load(); }

private:
  std::vector<T> store;
  std::atomic<size_t> next_write_index = {0};
  std::atomic<size_t> next_read_index = {0};
  std::atomic<size_t> num_elements = {0};
};
}  // namespace Common