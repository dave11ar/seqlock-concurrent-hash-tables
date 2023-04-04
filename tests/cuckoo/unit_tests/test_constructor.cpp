#include <array>
#include <cmath>
#include <stdexcept>

#include <gtest/gtest.h>
#include "unit_test_util.hpp"

TEST(Constructor, DefaultSize) {
  IntIntTable tbl;
  ASSERT_EQ(tbl.size(), 0);
  ASSERT_TRUE(tbl.empty());
  if (cuckoo_seqlock::DEFAULT_SIZE < 4) {
    ASSERT_EQ(tbl.hashpower(), 0);
  } else {
    ASSERT_EQ(tbl.hashpower(), (size_t)log2(static_cast<double>(cuckoo_seqlock::DEFAULT_SIZE) / 4));
  }
  ASSERT_EQ(tbl.bucket_count(), 1UL << tbl.hashpower());
  ASSERT_EQ(tbl.load_factor(), 0);
}

TEST(Constructor, GivenSize) {
  IntIntTable tbl(1);
  ASSERT_EQ(tbl.size(), 0);
  ASSERT_TRUE(tbl.empty());
  ASSERT_EQ(tbl.hashpower(), 0);
  ASSERT_EQ(tbl.bucket_count(), 1);
  ASSERT_EQ(tbl.load_factor(), 0);
}

TEST(Constructor, FreesEvenWithExceptions) {
  typedef IntIntTableWithAlloc<TrackingAllocator<int, 0>> no_space_table;
  // Should throw when allocating anything
  ASSERT_THROW(no_space_table(1), std::bad_alloc);
  ASSERT_EQ(get_unfreed_bytes(), 0);

  typedef IntIntTableWithAlloc<
      TrackingAllocator<int, cuckoo_seqlock::UnitTestInternalAccess::IntIntBucketSize>>
      some_space_table;
  // Should throw when allocating things after the bucket
  ASSERT_THROW(some_space_table(1), std::bad_alloc);
  ASSERT_EQ(get_unfreed_bytes(), 0);
}

struct StatefulHash {
  StatefulHash(int state_) : state(state_) {}
  size_t operator()(int x) const { return x; }
  int state;
};

struct StatefulKeyEqual {
  StatefulKeyEqual(int state_) : state(state_) {}
  bool operator()(int x, int y) const { return x == y; }
  int state;
};

template <typename T> struct StatefulAllocator {
  using value_type = T;
  using pointer = T *;
  using const_pointer = const T *;
  using reference = T &;
  using const_reference = const T &;
  using size_type = size_t;
  using difference_type = ptrdiff_t;

  template <typename U> struct rebind { using other = StatefulAllocator<U>; };

  StatefulAllocator() : state(0) {}

  StatefulAllocator(int state_) : state(state_) {}

  template <typename U>
  StatefulAllocator(const StatefulAllocator<U> &other) : state(other.state) {}

  T *allocate(size_t n) { return std::allocator<T>().allocate(n); }

  void deallocate(T *p, size_t n) { std::allocator<T>().deallocate(p, n); }

  template <typename U, class... Args> void construct(U *p, Args &&... args) {
    new ((void *)p) U(std::forward<Args>(args)...);
  }

  template <typename U> void destroy(U *p) { p->~U(); }

  StatefulAllocator select_on_container_copy_construction() const {
    return StatefulAllocator();
  }

  using propagate_on_container_swap = std::integral_constant<bool, true>;

  int state;
};

template <typename T, typename U>
bool operator==(const StatefulAllocator<T> &a1,
                const StatefulAllocator<U> &a2) {
  return a1.state == a2.state;
}

template <typename T, typename U>
bool operator!=(const StatefulAllocator<T> &a1,
                const StatefulAllocator<U> &a2) {
  return a1.state != a2.state;
}

using alloc_t = StatefulAllocator<std::pair<const int, int>>;
using tbl_t =
    cuckoo_seqlock::cuckoohash_map<int, int, StatefulHash, StatefulKeyEqual, alloc_t, 4>;

TEST(Constructor, StatefulComponents) {
  tbl_t map(8, StatefulHash(10), StatefulKeyEqual(20), alloc_t(30));
  ASSERT_EQ(map.hash_function().state, 10);
  for (int i = 0; i < 100; ++i) {
    ASSERT_EQ(map.hash_function()(i), i);
  }
  ASSERT_EQ(map.key_eq().state, 20);
  for (int i = 0; i < 100; ++i) {
    ASSERT_TRUE(map.key_eq()(i, i));
    ASSERT_FALSE(map.key_eq()(i, i + 1));
  }
  ASSERT_EQ(map.get_allocator().state, 30);
}

TEST(Constructor, RangeConstructor) {
  std::array<typename alloc_t::value_type, 3> elems{{{1, 2}, {3, 4}, {5, 6}}};
  tbl_t map(elems.begin(), elems.end(), 3, StatefulHash(10),
            StatefulKeyEqual(20), alloc_t(30));
  ASSERT_EQ(map.hash_function().state, 10);
  ASSERT_EQ(map.key_eq().state, 20);
  ASSERT_EQ(map.get_allocator().state, 30);
  for (int i = 1; i <= 5; i += 2) {
    ASSERT_EQ(map.find(i), i + 1);
  }
}

TEST(Constructor, CopyConstructor) {
  tbl_t map(0, StatefulHash(10), StatefulKeyEqual(20), alloc_t(30));
  ASSERT_EQ(map.get_allocator().state, 30);
  tbl_t map2(map);
  ASSERT_EQ(map2.hash_function().state, 10);
  ASSERT_EQ(map2.key_eq().state, 20);
  ASSERT_EQ(map2.get_allocator().state, 0);
}

TEST(Constructor, CopyConstructorOtherAllocator) {
  tbl_t map(0, StatefulHash(10), StatefulKeyEqual(20), alloc_t(30));
  tbl_t map2(map, map.get_allocator());
  ASSERT_EQ(map2.hash_function().state, 10);
  ASSERT_EQ(map2.key_eq().state, 20);
  ASSERT_EQ(map2.get_allocator().state, 30);
}

TEST(Constructor, MoveConstructor) {
  tbl_t map(10, StatefulHash(10), StatefulKeyEqual(20), alloc_t(30));
  map.insert(10, 10);
  tbl_t map2(std::move(map));
  ASSERT_EQ(map.size(), 0);
  ASSERT_EQ(map2.size(), 1);
  ASSERT_EQ(map2.hash_function().state, 10);
  ASSERT_EQ(map2.key_eq().state, 20);
  ASSERT_EQ(map2.get_allocator().state, 30);
}

TEST(Constructor, MoveConstructorDifferentAllocator) {
  tbl_t map(10, StatefulHash(10), StatefulKeyEqual(20), alloc_t(30));
  map.insert(10, 10);
  tbl_t map2(std::move(map), alloc_t(40));
  ASSERT_EQ(map.size(), 1);
  ASSERT_EQ(map.hash_function().state, 10);
  ASSERT_EQ(map.key_eq().state, 20);
  ASSERT_EQ(map.get_allocator().state, 30);

  ASSERT_EQ(map2.size(), 1);
  ASSERT_EQ(map2.hash_function().state, 10);
  ASSERT_EQ(map2.key_eq().state, 20);
  ASSERT_EQ(map2.get_allocator().state, 40);
}

TEST(Constructor, InitializerListConstructor) {
  tbl_t map({{1, 2}, {3, 4}, {5, 6}}, 3, StatefulHash(10), StatefulKeyEqual(20),
            alloc_t(30));
  ASSERT_EQ(map.hash_function().state, 10);
  ASSERT_EQ(map.key_eq().state, 20);
  ASSERT_EQ(map.get_allocator().state, 30);
  for (int i = 1; i <= 5; i += 2) {
    ASSERT_EQ(map.find(i), i + 1);
  }
}

TEST(Constructor, SwapMaps) {
  tbl_t map({{1, 2}}, 1, StatefulHash(10), StatefulKeyEqual(20), alloc_t(30));
  tbl_t map2({{3, 4}}, 1, StatefulHash(40), StatefulKeyEqual(50), alloc_t(60));
  map.swap(map2);

  ASSERT_EQ(map.size(), 1);
  ASSERT_EQ(map.hash_function().state, 40);
  ASSERT_EQ(map.key_eq().state, 50);
  ASSERT_EQ(map.get_allocator().state, 60);

  ASSERT_EQ(map2.size(), 1);
  ASSERT_EQ(map2.hash_function().state, 10);
  ASSERT_EQ(map2.key_eq().state, 20);
  ASSERT_EQ(map2.get_allocator().state, 30);

  // // Uses ADL to find the specialized swap.
  swap(map, map2);

  ASSERT_EQ(map.size(), 1);
  ASSERT_EQ(map.hash_function().state, 10);
  ASSERT_EQ(map.key_eq().state, 20);
  ASSERT_EQ(map.get_allocator().state, 30);

  ASSERT_EQ(map2.size(), 1);
  ASSERT_EQ(map2.hash_function().state, 40);
  ASSERT_EQ(map2.key_eq().state, 50);
  ASSERT_EQ(map2.get_allocator().state, 60);
}

TEST(Constructor, CopyAssignDifferentAllocators) {
  tbl_t map({{1, 2}}, 1, StatefulHash(10), StatefulKeyEqual(20), alloc_t(30));
  tbl_t map2({{3, 4}}, 1, StatefulHash(40), StatefulKeyEqual(50), alloc_t(60));

  map = map2;
  ASSERT_EQ(map.size(), 1);
  ASSERT_EQ(map.find(3), 4);
  ASSERT_EQ(map.hash_function().state, 40);
  ASSERT_EQ(map.key_eq().state, 50);
  ASSERT_EQ(map.get_allocator().state, 30);

  ASSERT_EQ(map2.size(), 1);
  ASSERT_EQ(map2.hash_function().state, 40);
  ASSERT_EQ(map2.key_eq().state, 50);
  ASSERT_EQ(map2.get_allocator().state, 60);
}

TEST(Constructor, MoveAssignDifferentAllocators) {
  tbl_t map({{1, 2}}, 1, StatefulHash(10), StatefulKeyEqual(20), alloc_t(30));
  tbl_t map2({{3, 4}}, 1, StatefulHash(40), StatefulKeyEqual(50), alloc_t(60));

  map = std::move(map2);
  ASSERT_EQ(map.size(), 1);
  ASSERT_EQ(map.find(3), 4);
  ASSERT_EQ(map.hash_function().state, 40);
  ASSERT_EQ(map.key_eq().state, 50);
  ASSERT_EQ(map.get_allocator().state, 30);

  ASSERT_EQ(map2.hash_function().state, 40);
  ASSERT_EQ(map2.key_eq().state, 50);
  ASSERT_EQ(map2.get_allocator().state, 60);
}

TEST(Constructor, MoveAssignSameAllocators) {
  tbl_t map({{1, 2}}, 1, StatefulHash(10), StatefulKeyEqual(20), alloc_t(30));
  tbl_t map2({{3, 4}}, 1, StatefulHash(40), StatefulKeyEqual(50), alloc_t(30));

  map = std::move(map2);
  ASSERT_EQ(map.size(), 1);
  ASSERT_EQ(map.find(3), 4);
  ASSERT_EQ(map.hash_function().state, 40);
  ASSERT_EQ(map.key_eq().state, 50);
  ASSERT_EQ(map.get_allocator().state, 30);

  ASSERT_EQ(map2.size(), 0);
  ASSERT_EQ(map2.hash_function().state, 40);
  ASSERT_EQ(map2.key_eq().state, 50);
  ASSERT_EQ(map2.get_allocator().state, 30);
}

TEST(Constructor, InitializerListAssignment) {
  tbl_t map({{1, 2}}, 1, StatefulHash(10), StatefulKeyEqual(20), alloc_t(30));
  ASSERT_EQ(map.find(1), 2);
  map = {{3, 4}};
  ASSERT_EQ(map.find(3), 4);
}
