#pragma once

#include <atomic>
#include <limits>

#include "utils.hpp"

namespace seqlock_lib {

using seqlock_epoch_t = uint64_t;

LIBCUCKOO_SQUELCH_PADDING_WARNING
class LIBCUCKOO_ALIGNAS(64) seqlock {
private:
  static constexpr seqlock_epoch_t seqlock_type_size = sizeof(seqlock_epoch_t) << 3;

public:
  static constexpr seqlock_epoch_t lock_bit = seqlock_epoch_t(1) << (seqlock_type_size - 1);

private:
  static constexpr seqlock_epoch_t is_migrated_bit = lock_bit >> 1;
  static constexpr seqlock_epoch_t is_migrated_bit_xor = 
    std::numeric_limits<seqlock_epoch_t>::max() ^ is_migrated_bit;

public:
  seqlock(bool blocked = false, bool is_migrated = true) noexcept
      : elem_counter_(0), cur_epoch_((is_migrated ? is_migrated_bit : 0)) {
    if (blocked) { 
      lock_.test_and_set(std::memory_order_acq_rel);
    }
    epoch_.store(
      (blocked ? lock_bit : 0) + (is_migrated ? is_migrated_bit : 0), 
      std::memory_order_release);
  }

  seqlock(const seqlock &other) noexcept
      : elem_counter_(other.elem_counter_), cur_epoch_(other.cur_epoch_) {
    epoch_.store(other.epoch_.load(std::memory_order_acquire), std::memory_order_release);
    if (other.lock_.test(std::memory_order_acquire)) {
      lock_.test_and_set(std::memory_order_acq_rel);
    }
  }

  inline seqlock &operator=(const seqlock &other) noexcept {
    elem_counter_ = other.elem_counter_;
    cur_epoch_ = other.cur_epoch_;
    epoch_.store(other.epoch_.load(std::memory_order_acquire), std::memory_order_release);
    if (other.lock_.test(std::memory_order_acquire)) {
      lock_.test_and_set(std::memory_order_acq_rel);
    } else {
      lock_.clear(std::memory_order_release);
    }

    return *this;
  }

  inline seqlock_epoch_t lock() noexcept {
    while (lock_.test_and_set(std::memory_order_acq_rel));

    const seqlock_epoch_t res = ++cur_epoch_ | lock_bit;
    epoch_.store(res, std::memory_order_release);

    return res;
  }

  inline bool try_lock() noexcept {
    if (!lock_.test_and_set(std::memory_order_acq_rel)) {
      epoch_.store(++cur_epoch_ | lock_bit, std::memory_order_release);
      return true;
    }

    return false;
  }

  inline void unlock() noexcept {
    ++cur_epoch_;
    unlock_atomics();
  }

  inline void unlock_no_modified() noexcept {
    --cur_epoch_;
    unlock_atomics();
  }

  inline seqlock_epoch_t get_epoch() const noexcept {
    return epoch_.load(std::memory_order_acquire);
  }

  constexpr static inline bool is_locked(seqlock_epoch_t value) noexcept {
    return (value & lock_bit) != 0;
  }

  constexpr static inline bool is_migrated(seqlock_epoch_t value) noexcept {
    return (value & is_migrated_bit) != 0;
  }

  inline void set_migrated(bool is_migrated) noexcept {
    if (is_migrated) {
      cur_epoch_ |= is_migrated_bit;
    } else {
      cur_epoch_ &= is_migrated_bit_xor;
    }

    epoch_.store(cur_epoch_, std::memory_order_release);
  }

  inline counter_type elem_counter() const noexcept {
    return elem_counter_;
  }
  inline counter_type& elem_counter() noexcept {
    return elem_counter_;
  }

private:
  inline void unlock_atomics() noexcept {
    std::atomic_thread_fence(std::memory_order_release);
    epoch_.store(cur_epoch_, std::memory_order_relaxed);
    lock_.clear(std::memory_order_relaxed);
  }

  counter_type elem_counter_;

  // always has lock_bit = 0 
  seqlock_epoch_t cur_epoch_;
  
  std::atomic<seqlock_epoch_t> epoch_;
  std::atomic_flag lock_;
};

}  // namespace seqlock_lib
