#include <array>
#include <limits>

#include <gtest/gtest.h>

#include "unit_test_util.hpp"

using cuckoo_seqlock::UnitTestInternalAccess;

TEST(RehashEmptyTable, Resize) {
  IntIntTable table(1);
  ASSERT_EQ(table.hashpower(), 0);

  table.rehash(20);
  ASSERT_EQ(table.hashpower(), 20);

  table.rehash(1);
  ASSERT_EQ(table.hashpower(), 1);
}

TEST(ReserveEmptyTable, Resize) {
  IntIntTable table(1);
  table.reserve(100);
  ASSERT_EQ(table.hashpower(), 5);

  table.reserve(1);
  ASSERT_EQ(table.hashpower(), 0);

  table.reserve(2);
  ASSERT_EQ(table.hashpower(), 0);
}

TEST(ReserveCalc, Resize) {
  const size_t slot_per_bucket = IntIntTable::slot_per_bucket();
  ASSERT_EQ(UnitTestInternalAccess::reserve_calc<IntIntTable>(0), 0);
  ASSERT_EQ(UnitTestInternalAccess::reserve_calc<IntIntTable>(
              1 * slot_per_bucket), 0);

  ASSERT_EQ(UnitTestInternalAccess::reserve_calc<IntIntTable>(
              2 * slot_per_bucket), 1);
  ASSERT_EQ(UnitTestInternalAccess::reserve_calc<IntIntTable>(
              3 * slot_per_bucket), 2);
  ASSERT_EQ(UnitTestInternalAccess::reserve_calc<IntIntTable>(
              4 * slot_per_bucket), 2);
  ASSERT_EQ(UnitTestInternalAccess::reserve_calc<IntIntTable>(
              2500000 * slot_per_bucket), 22);

  // The maximum number of elements we can ask to reserve without incurring
  // rounding error when computing a number of buckets is
  // SIZE_T_MAX-slot_per_bucket(), which will come out to int_div(SIZE_T_MAX -
  // 1, slot_per_bucket()) buckets.
  const size_t max_buckets = (
      std::numeric_limits<size_t>::max() - 1)/slot_per_bucket;
  // Since the table is always sized in powers of two, our maximum hashpower
  // comes out to max_hashpower = floor(log2(max_buckets)). We compute this in
  // a numerically-stable fashion.
  size_t max_hashpower = 0;
  for (; (static_cast<size_t>(1) << (max_hashpower + 1)) <= max_buckets; ++max_hashpower);
  // Test the boundary between max_hashpower-1 and max_hashpower.
  const size_t max_elems_before_max_hashpower = (
      static_cast<size_t>(1) << (max_hashpower - 1)) * slot_per_bucket;
  ASSERT_EQ(UnitTestInternalAccess::reserve_calc<IntIntTable>(
              max_elems_before_max_hashpower), (max_hashpower - 1));
  ASSERT_EQ(UnitTestInternalAccess::reserve_calc<IntIntTable>(
              max_elems_before_max_hashpower + 1), max_hashpower);
  // Test the maximum number of elements.
  const size_t max_elems = (static_cast<size_t>(1) << max_hashpower) * slot_per_bucket;
  ASSERT_EQ(UnitTestInternalAccess::reserve_calc<IntIntTable>(max_elems), max_hashpower);
}

struct my_type {
  int x;
  my_type(int v) : x(v) {}
  my_type(const my_type& other) {
    x = other.x;
  }
  my_type(my_type&& other) {
    x = other.x;
    ++num_moves;
  }
  ~my_type() { ++num_deletes; }
  static size_t num_deletes;
  static size_t num_moves;
};

size_t my_type::num_deletes = 0;
size_t my_type::num_moves = 0;

TEST(ResizingNumberOfFrees, Resize) {
  my_type val(0);
  size_t num_deletes_after_resize;
  {
    // Should allocate 2 buckets of 4 slots
    cuckoo_seqlock::cuckoohash_map<int, my_type, std::hash<int>, std::equal_to<int>,
                   std::allocator<std::pair<const int, my_type>>, 4>
        map(8);
    for (int i = 0; i < 9; ++i) {
      map.insert(i, val);
    }

    // Items which should be moved from old bucket to new
    // 2, 6: 0 -> 2
    // 1, 3, 5, 7: 1 -> 3
    ASSERT_EQ(my_type::num_deletes, 6);
    ASSERT_EQ(my_type::num_moves, 6);
  }
  ASSERT_EQ(my_type::num_deletes, 15);
}

// Taken from https://github.com/facebook/folly/blob/master/folly/docs/Traits.md
class NonRelocatableType {
public:
  std::array<char, 1024> buffer;
  char *pointerToBuffer;
  NonRelocatableType() : pointerToBuffer(buffer.data()) {}
  NonRelocatableType(char c) : pointerToBuffer(buffer.data()) {
    buffer.fill(c);
  }

  NonRelocatableType(const NonRelocatableType &x) noexcept
      : buffer(x.buffer), pointerToBuffer(buffer.data()) {}

  NonRelocatableType &operator=(const NonRelocatableType &x) {
    buffer = x.buffer;
    return *this;
  }
};

TEST(ResizeOnNonRelocatableType, Resize) {
  cuckoo_seqlock::cuckoohash_map<int, NonRelocatableType, std::hash<int>, std::equal_to<int>,
                 std::allocator<std::pair<const int, NonRelocatableType>>, 1>
      map(0);
  ASSERT_EQ(map.hashpower(), 0);
  // Make it resize a few times to ensure the vector capacity has to actually
  // change when we resize the buckets
  const size_t num_elems = 16;
  for (int i = 0; i < num_elems; ++i) {
    map.insert(i, 'a');
  }
  // Make sure each pointer actually points to its buffer
  NonRelocatableType value;
  std::array<char, 1024> ref;
  ref.fill('a');
  auto lt = map.lock_table();
  for (const auto &kvpair : lt) {
    ASSERT_EQ(ref, kvpair.second.buffer);
    ASSERT_EQ(kvpair.second.pointerToBuffer, kvpair.second.buffer.data());
  }
}
