#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "Macros.hpp"

namespace Common {
template <typename T>
class MemPool final {
 public:
  explicit MemPool(std::size_t num_elems) : store_(num_elems, {T(), true}) {
    // std::cout << (reinterpret_cast<const ObjectBlock *> (&(store_[0].object)) == &(store_[0])) <<
    // '\n';
    ASSERT(reinterpret_cast<const ObjectBlock *>(&(store_[0].object)) == &(store_[0]),
           "T object should be the first member of Object Block");
  }
  MemPool() = delete;
  MemPool(const MemPool &) = delete;
  MemPool(const MemPool &&) = delete;
  MemPool &operator=(const MemPool &) = delete;
  MemPool &operator=(const MemPool &&) = delete;

  template <typename... Args>
  T *allocate(Args... args) noexcept {
    auto object_block = &(store_[next_free_index_]);
    ASSERT(object_block->is_free,
           "Expected free block at index : " + std::to_string(next_free_index_));
    T *ret = &(object_block->object);
    ret = new (ret) T(args...);  // Construct of new object of type T on the memory address of the
                                 // current one (overwrite it)
    object_block->is_free = false;
    updateNextFreeIndex();
    return ret;
  }

  auto deallocate(const T *elem) noexcept {
    const auto elem_index = (reinterpret_cast<const ObjectBlock *>(elem) - &store_[0]);
    ASSERT(elem_index >= 0 && static_cast<size_t>(elem_index) < store_.size(),
           "Element being deallocated does not belong to this memory pool. ");
    ASSERT(!store_[elem_index].is_free,
           "Expected in use Object block at index : " + std::to_string(elem_index));
    store_[elem_index].is_free = true;
  }

 private:
  auto updateNextFreeIndex() noexcept {
    const auto initial_free_index = next_free_index_;
    while (!store_[next_free_index_].is_free) {
      ++next_free_index_;
      if (UNLIKELY(next_free_index_ == store_.size())) {
        next_free_index_ = 0;  // Unlikely to happen
      }
      if (UNLIKELY(initial_free_index == next_free_index_)) {
        ASSERT(initial_free_index != next_free_index_, "Memory Pool out of space.");
      }
    }
  }

  struct ObjectBlock {
    T object;
    bool is_free = true;
  };
  std::vector<ObjectBlock> store_;
  size_t next_free_index_ = 0;
};
};  // namespace Common