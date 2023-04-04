#pragma once

#include <exception>
#include <thread>

#include "cuckoohash_config.hpp" // for LIBCUCKOO_DEBUG

namespace cuckoo_seqlock {

#if LIBCUCKOO_DEBUG
//! When \ref LIBCUCKOO_DEBUG is 0, LIBCUCKOO_DBG will printing out status
//! messages in various situations
#define LIBCUCKOO_DBG(fmt, ...)                                                \
  fprintf(stderr, "\x1b[32m"                                                   \
                  "[libcuckoo:%s:%d:%lu] " fmt ""                              \
                  "\x1b[0m",                                                   \
          __FILE__, __LINE__,                                                  \
          std::hash<std::thread::id>()(std::this_thread::get_id()),            \
          __VA_ARGS__)
#else
//! When \ref LIBCUCKOO_DEBUG is 0, LIBCUCKOO_DBG does nothing
#define LIBCUCKOO_DBG(fmt, ...)                                                \
  do {                                                                         \
  } while (0)
#endif

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

/**
 * At higher warning levels, MSVC may issue a deadcode warning which depends on
 * the template arguments given. For certain other template arguments, the code
 * is not really "dead".
 */
#ifdef _MSC_VER
#define LIBCUCKOO_SQUELCH_DEADCODE_WARNING_BEGIN                               \
  do {                                                                         \
    __pragma(warning(push));                                                   \
    __pragma(warning(disable : 4702))                                          \
  } while (0)
#define LIBCUCKOO_SQUELCH_DEADCODE_WARNING_END __pragma(warning(pop))
#else
#define LIBCUCKOO_SQUELCH_DEADCODE_WARNING_BEGIN
#define LIBCUCKOO_SQUELCH_DEADCODE_WARNING_END
#endif

/**
 * Thrown when an automatic expansion is triggered, but the load factor of the
 * table is below a minimum threshold, which can be set by the \ref
 * cuckoohash_map::minimum_load_factor method. This can happen if the hash
 * function does not properly distribute keys, or for certain adversarial
 * workloads.
 */
class load_factor_too_low : public std::exception {
public:
  /**
   * Constructor
   *
   * @param lf the load factor of the table when the exception was thrown
   */
  load_factor_too_low(const double lf) noexcept : load_factor_(lf) {}

  /**
   * @return a descriptive error message
   */
  virtual const char *what() const noexcept override {
    return "Automatic expansion triggered when load factor was below "
           "minimum threshold";
  }

  /**
   * @return the load factor of the table when the exception was thrown
   */
  double load_factor() const noexcept { return load_factor_; }

private:
  const double load_factor_;
};

/**
 * Thrown when an expansion is triggered, but the hashpower specified is greater
 * than the maximum, which can be set with the \ref
 * cuckoohash_map::maximum_hashpower method.
 */
class maximum_hashpower_exceeded : public std::exception {
public:
  /**
   * Constructor
   *
   * @param hp the hash power we were trying to expand to
   */
  maximum_hashpower_exceeded(const size_t hp) noexcept : hashpower_(hp) {}

  /**
   * @return a descriptive error message
   */
  virtual const char *what() const noexcept override {
    return "Expansion beyond maximum hashpower";
  }

  /**
   * @return the hashpower we were trying to expand to
   */
  size_t hashpower() const noexcept { return hashpower_; }

private:
  const size_t hashpower_;
};

template <typename Allocator, typename U>
using rebind_alloc =
  typename std::allocator_traits<Allocator>::template rebind_alloc<U>;

// Counter type
using counter_type = int64_t;

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

inline void cpu_relax() {
#if defined(__x86_64)
  asm volatile("pause\n" : : : "memory");
#elif defined(_M_AMD64)
  asm volatile("yield\n" : : : "memory");
#endif
}

template <typename size_type>
constexpr size_type hashsize(size_type hp) {
  return size_type(1) << hp;
}

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

}  // namespace cuckoo_seqlock


