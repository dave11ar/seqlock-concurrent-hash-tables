#include <array>
#include <limits>

#include <gtest/gtest.h>

#include "unit_test_util.hpp"

using seqlock_lib::cuckoo::UnitTestInternalAccess;

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
};
