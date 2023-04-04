#pragma once

#include "cuckoohash_util.hpp"
#include <atomic>
#include <iostream>
namespace cuckoo_seqlock {

using seqlock_lock_type = uint32_t;

LIBCUCKOO_SQUELCH_PADDING_WARNING
class LIBCUCKOO_ALIGNAS(64) seqlock {
public:
  seqlock(bool blocked = false, bool is_migrated = true)
      : elem_counter_(0), is_migrated_(is_migrated) {
    lock_.store(blocked, std::memory_order_release);
  }

  seqlock(const seqlock &other) noexcept
      : elem_counter_(other.elem_counter()), is_migrated_(other.is_migrated()) {
    lock_.store(other.lock_.load(std::memory_order_acquire), std::memory_order_release);
  }

  seqlock &operator=(const seqlock &other) noexcept {
    elem_counter() = other.elem_counter();
    is_migrated_.store(other.is_migrated());
    lock_.store(other.lock_.load(std::memory_order_acquire), std::memory_order_release);

    return *this;
  }

  void lock() noexcept {
    while (!try_lock()) {}
  }

  void unlock() noexcept {
    lock_.fetch_add(1, std::memory_order_acq_rel);
  }

  void unlock_no_modified() noexcept {
    lock_.fetch_sub(1, std::memory_order_acq_rel);
  }

  bool try_lock() noexcept {
    seqlock_lock_type lock_value = lock_.load(std::memory_order_acquire);
    return ((lock_value & 1) != 0) ? false 
      : lock_.compare_exchange_strong(lock_value, lock_value + 1, std::memory_order_acq_rel);
  }

  seqlock_lock_type get_value() noexcept {
    return lock_.load(std::memory_order_acquire);
  }

  int64_t elem_counter() const noexcept { return elem_counter_; }
  int64_t& elem_counter() noexcept { return elem_counter_; }

  bool is_migrated() const noexcept {
    return is_migrated_.load(std::memory_order_acquire);
  }
  void set_migrated(bool is_migrated) noexcept { 
    is_migrated_.store(is_migrated, std::memory_order_release);
  }

private:
  int64_t elem_counter_;
  std::atomic<seqlock_lock_type> lock_;
  std::atomic<bool> is_migrated_;
};

}  // namespace cuckoo_seqlock

