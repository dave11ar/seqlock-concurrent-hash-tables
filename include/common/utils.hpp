#pragma once

#include <cassert>
#include "atomic_buffer.hpp"

namespace seqlock_lib {

/**
 * alignas() requires GCC >= 4.9, so we stick with the alignment attribute for
 * GCC.
 */
#ifdef __GNUC__
#define LIBCUCKOO_ALIGNAS(x) __attribute__((aligned(x)))
#else
#define LIBCUCKOO_ALIGNAS(x) alignas(x)
#endif

/**
 * At higher warning levels, MSVC produces an annoying warning that alignment
 * may cause wasted space: "structure was padded due to __declspec(align())".
 */
#ifdef _MSC_VER
#define LIBCUCKOO_SQUELCH_PADDING_WARNING __pragma(warning(suppress : 4324))
#else
#define LIBCUCKOO_SQUELCH_PADDING_WARNING
#endif

// Counter type
using counter_type = int64_t;

inline void cpu_relax() {
#if defined(__x86_64__)
  asm volatile("pause" : : : "memory");
#elif defined(__aarch64__)
  asm volatile("yield" : : : "memory");
#endif
}

// true here means the allocators from `src` are propagated on libcuckoo_copy
template <typename A>
void copy_allocator(A &dst, const A &src, std::true_type) {
  dst = src;
}

template <typename A>
void copy_allocator(A &, const A &, std::false_type) {}

// true here means the allocators from `src` are propagated on libcuckoo_swap
template <typename A> void swap_allocator(A &dst, A &src, std::true_type) {
  std::swap(dst, src);
}

template <typename A> void swap_allocator(A &, A &, std::false_type) {}

// reserve_calc takes in a parameter specifying a certain number of slots
// for a table and returns the smallest hashpower that will hold n elements.
template <size_t SLOT_PER_BUCKET, typename size_type>
static size_type reserve_calc_for_slots(const size_type n) {
  const size_type buckets = (n + SLOT_PER_BUCKET - 1) / SLOT_PER_BUCKET;
  size_type blog2;
  for (blog2 = 0; (size_type(1) << blog2) < buckets; ++blog2);
  
  assert(n <= buckets * SLOT_PER_BUCKET && buckets <= (size_type(1) << blog2));
  return blog2;
}

template <typename Lock>
struct LockDeleter {
  void operator()(Lock* l) const {l->unlock(); }
};

// This exception is thrown whenever we try to lock a bucket, but the
// hashpower is not what was expected
class hashpower_changed {};


// A small wrapper around std::atomic to make it copyable for constructors.
template <typename AtomicT>
class CopyableAtomic : public std::atomic<AtomicT> {
 public:
  using std::atomic<AtomicT>::atomic;

  CopyableAtomic(const CopyableAtomic& other) noexcept
      : CopyableAtomic(other.load(std::memory_order_acquire)) {}

  CopyableAtomic& operator=(const CopyableAtomic& other) noexcept {
    this->store(other.load(std::memory_order_acquire),
                std::memory_order_release);
    return *this;
  }
};

enum class operation_mode : bool {
  SAFE,
  UNSAFE
};

template <operation_mode OPERATION_MODE, typename mapped_type, typename F>
void update_safely(mapped_type& value, F fn) {
  if constexpr (OPERATION_MODE == operation_mode::SAFE) {
    mapped_type value_copy(value);
    fn(value_copy);
    atomic_store_memcpy(&value, value_copy);
  } else {
    fn(value);
  }
}

template <typename Type>
using aligned_storage_type =
  std::aligned_storage_t<sizeof(Type), alignof(Type)>;

} // seqlock_lib
