#include <iostream>

#include <gtest/gtest.h>

#include "unit_test_util.hpp"

TEST(MinimumLoadFactorInitializedToDefault, MinimumLoadFactor) {
  IntIntTable tbl;
  ASSERT_EQ(tbl.minimum_load_factor(), cuckoo_seqlock::DEFAULT_MINIMUM_LOAD_FACTOR);
}

class BadHashFunction {
public:
  size_t operator()(int) { return 0; }
};

TEST(CapsAutomaticExpansion, MinimumLoadFactor) {
  const size_t slot_per_bucket = 4;
  cuckoo_seqlock::cuckoohash_map<int, int, BadHashFunction, std::equal_to<int>,
                 std::allocator<std::pair<const int, int>>, slot_per_bucket>
      tbl(16);
  tbl.minimum_load_factor(0.6);

  for (size_t i = 0; i < 2 * slot_per_bucket; ++i) {
    tbl.insert(i, i);
  }

  ASSERT_THROW(tbl.insert(2 * slot_per_bucket, 0),
               cuckoo_seqlock::load_factor_too_low);
}

TEST(InvalidMinimumLoadFactor, MinimumLoadFactor) {
  IntIntTable tbl;
  ASSERT_THROW(tbl.minimum_load_factor(-0.01), std::invalid_argument);
  ASSERT_THROW(tbl.minimum_load_factor(1.01), std::invalid_argument);
}
