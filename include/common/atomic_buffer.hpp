#pragma once

#include <atomic>

namespace seqlock_lib {

template <size_t Align>
using big_type = std::conditional_t<Align == 8, uint8_t,
                 std::conditional_t<Align == 16, uint16_t,
                 std::conditional_t<Align == 32, uint32_t, 
                 std::conditional_t<Align == 64, uint64_t, char>>>>;

template <typename T>
constexpr size_t big_type_count = sizeof(T) / sizeof(big_type<alignof(T)>);
template <typename T>
constexpr size_t bytes_count = big_type_count<T> * sizeof(big_type<alignof(T)>);

template<typename T>
std::atomic<T>& as_atomic(T& t) {
  return reinterpret_cast<std::atomic<T>&>(t);
}

template<typename T>
const std::atomic<T>& as_atomic(const T& t) {
  return reinterpret_cast<const std::atomic<T>&>(t);
}

template <typename T>
void atomic_load_memcpy(T* dest, const T& source) {
  using big_type_ = big_type<alignof(T)>;

  for (size_t i = 0; i < big_type_count<T>; ++i) {
    auto& dst = reinterpret_cast<big_type_*>(dest)[i];
    auto& src = reinterpret_cast<const big_type_*>(&source)[i];

    dst = as_atomic(src).load(std::memory_order_relaxed);
  }
  for (size_t i = bytes_count<T>; i < sizeof(T); ++i) {
    auto& dst = reinterpret_cast<char*>(dest)[i];
    auto& src = reinterpret_cast<const char*>(&source)[i];

    dst = as_atomic(src).load(std::memory_order_relaxed);
  }
}

template <typename T>
void atomic_store_memcpy(T* dest, const T& source) {
  using big_type_ = big_type<alignof(T)>;

  for (size_t i = 0; i < big_type_count<T>; ++i) {
    auto& dst = reinterpret_cast<big_type_*>(dest)[i];
    auto& src = reinterpret_cast<const big_type_*>(&source)[i];

    as_atomic(dst).store(src, std::memory_order_relaxed);
  }
  for (size_t i = bytes_count<T>; i < sizeof(T); ++i) {
    auto& dst = reinterpret_cast<char*>(dest)[i];
    auto& src = reinterpret_cast<const char*>(&source)[i];

    as_atomic(dst).store(src, std::memory_order_relaxed);
  }
}

} // namespace seqlock_lib
