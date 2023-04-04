#include <iostream>

#include <gtest/gtest.h>

#include "unit_test_util.hpp"

TEST(MaximumHashpowerInitializedToDefault, MaximumHashPower) {
  IntIntTable tbl;
  ASSERT_EQ(tbl.maximum_hashpower(), cuckoo_seqlock::NO_MAXIMUM_HASHPOWER);
}

TEST(CapsAnyExpansion, MaximumHashPower) {
  IntIntTable tbl(1);
  tbl.maximum_hashpower(1);
  for (size_t i = 0; i < 2 * tbl.slot_per_bucket(); ++i) {
    tbl.insert(i, i);
  }

  ASSERT_EQ(tbl.hashpower(), 1);
  ASSERT_THROW(tbl.insert(2 * tbl.slot_per_bucket(), 0),
               cuckoo_seqlock::maximum_hashpower_exceeded);
  ASSERT_THROW(tbl.rehash(2), cuckoo_seqlock::maximum_hashpower_exceeded);
  ASSERT_THROW(tbl.reserve(4 * tbl.slot_per_bucket()),
               cuckoo_seqlock::maximum_hashpower_exceeded);
}

TEST(NoMaximumHashPower, MaximumHashPower) {
  // It's difficult to check that we actually don't ever set a maximum hash
  // power, but if we explicitly unset it, we should be able to expand beyond
  // the limit that we had previously set.
  IntIntTable tbl(1);
  tbl.maximum_hashpower(1);
  ASSERT_THROW(tbl.rehash(2), cuckoo_seqlock::maximum_hashpower_exceeded);

  tbl.maximum_hashpower(2);
  tbl.rehash(2);
  ASSERT_EQ(tbl.hashpower(), 2);
  ASSERT_THROW(tbl.rehash(3), cuckoo_seqlock::maximum_hashpower_exceeded);

  tbl.maximum_hashpower(cuckoo_seqlock::NO_MAXIMUM_HASHPOWER);
  tbl.rehash(10);
  ASSERT_EQ(tbl.hashpower(), 10);
}
