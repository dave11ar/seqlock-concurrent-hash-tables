#include <gtest/gtest.h>

#include "unit_test_util.hpp"

using seqlock_lib::cuckoo::UnitTestInternalAccess;

// Checks that the alt index function returns a different bucket, and can
// recover the old bucket when called with the alternate bucket as the index.
template <class CuckoohashMap>
void check_key(size_t hashpower, const typename CuckoohashMap::key_type &key) {
  auto hashfn = typename CuckoohashMap::hasher();
  size_t hv = hashfn(key);
  auto partial = UnitTestInternalAccess::partial_key<CuckoohashMap>(hv);
  size_t bucket =
      UnitTestInternalAccess::index_hash<CuckoohashMap>(hashpower, hv);
  size_t alt_bucket = UnitTestInternalAccess::alt_index<CuckoohashMap>(
      hashpower, partial, bucket);
  size_t orig_bucket = UnitTestInternalAccess::alt_index<CuckoohashMap>(
      hashpower, partial, alt_bucket);

  ASSERT_NE(bucket, alt_bucket);
  ASSERT_EQ(bucket, orig_bucket);
}

TEST(HashProperties, IntAltIndexWorksCorrectly) {
  for (size_t hashpower = 10; hashpower < 15; ++hashpower) {
    for (int key = 0; key < 10000; ++key) {
      check_key<IntIntTable>(hashpower, key);
    }
  }
}

TEST(HashProperties, ArrayIntAltIndexWorksCorrectly) {
  for (size_t hashpower = 10; hashpower < 15; ++hashpower) {
    for (int key = 0; key < 10000; ++key) {
      ArrayT array;
      for (size_t i = 0; i < array.size(); ++i) {
        array[i] = key + i;
      }
      check_key<ArrayIntIntTable>(hashpower, array);
    }
  }
}

TEST(HashProperties, HashWithLargerHashpowerOnlyAddsTopBits) {
  ArrayT key;
  for (size_t i = 0; i < key.size(); ++i) {
    key[i] = 1337 + i;
  }
  
  size_t hv = ArrayIntIntTable::hasher()(key);
  for (size_t hashpower = 1; hashpower < 30; ++hashpower) {
    auto partial = UnitTestInternalAccess::partial_key<ArrayIntIntTable>(hv);
    size_t index_bucket1 =
        UnitTestInternalAccess::index_hash<ArrayIntIntTable>(hashpower, hv);
    size_t index_bucket2 =
        UnitTestInternalAccess::index_hash<ArrayIntIntTable>(hashpower + 1, hv);
    ASSERT_EQ((index_bucket2 & ~(1L << hashpower)), index_bucket1);

    size_t alt_bucket1 = UnitTestInternalAccess::alt_index<ArrayIntIntTable>(
        hashpower, partial, index_bucket1);
    size_t alt_bucket2 = UnitTestInternalAccess::alt_index<ArrayIntIntTable>(
        hashpower, partial, index_bucket2);

    ASSERT_EQ((alt_bucket2 & ~(1L << hashpower)), alt_bucket1);
  }
}
