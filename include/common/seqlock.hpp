#pragma once

#include <limits>

#include "utils.hpp"

namespace seqlock_lib {

using seqlock_epoch_t = uint64_t;

// Can be rewritten using bts assembler instruction, see
// https://github.com/oneapi-src/oneTBB/pull/131
template <size_t Align>
LIBCUCKOO_SQUELCH_PADDING_WARNING
class LIBCUCKOO_ALIGNAS(Align) seqlock {
private:
  static constexpr seqlock_epoch_t seqlock_type_size = sizeof(seqlock_epoch_t) << 3;

  static constexpr seqlock_epoch_t is_migrated_bit = seqlock_epoch_t(1) << (seqlock_type_size - 1);
  static constexpr seqlock_epoch_t is_migrated_bit_xor = 
    std::numeric_limits<seqlock_epoch_t>::max() ^ is_migrated_bit;

public:
  seqlock(bool blocked = false, bool is_migrated = true) noexcept
      : elem_counter_(0), cur_epoch_((blocked ? 1 : 0) + (is_migrated ? is_migrated_bit : 0)),
        epoch_(cur_epoch_) {
    // since C++20 default constructor initialize atomic_flag to clear state
    if (blocked) {
      lock_.test_and_set(std::memory_order_acquire);
    }
  }

  inline seqlock_epoch_t lock() noexcept {
    while (lock_.test_and_set(std::memory_order_acquire)) {
      cpu_relax();
    }

    epoch_.store(++cur_epoch_, std::memory_order_relaxed);

    return cur_epoch_;
  }

  inline bool try_lock() noexcept {
    if (!lock_.test_and_set(std::memory_order_acquire)) {
      epoch_.store(++cur_epoch_, std::memory_order_relaxed);
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

  inline seqlock_epoch_t get_epoch(std::memory_order m = std::memory_order_acquire) const noexcept {
    return epoch_.load(m);
  }

  constexpr static inline bool is_locked(seqlock_epoch_t value) noexcept {
    return (value & 1) != 0;
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

    epoch_.store(cur_epoch_, std::memory_order_relaxed);
  }

  inline counter_type elem_counter() const noexcept {
    return elem_counter_;
  }
  inline counter_type& elem_counter() noexcept {
    return elem_counter_;
  }

private:
  inline void unlock_atomics() noexcept {
    epoch_.store(cur_epoch_, std::memory_order_relaxed);
    lock_.clear(std::memory_order_release);
  }

  counter_type elem_counter_;

  seqlock_epoch_t cur_epoch_;
  std::atomic<seqlock_epoch_t> epoch_;
  std::atomic_flag lock_;
};

}  // namespace seqlock_lib
