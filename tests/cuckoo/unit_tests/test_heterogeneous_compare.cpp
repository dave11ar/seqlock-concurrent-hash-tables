#include <gtest/gtest.h>

#include "unit_test_util.hpp"

size_t int_constructions;
size_t foo_comparisons;
size_t int_comparisons;
size_t foo_hashes;
size_t int_hashes;

class Foo {
public:
  int val;

  Foo(int v) {
    ++int_constructions;
    val = v;
  }
};

class foo_eq {
public:
  bool operator()(const Foo &left, const Foo &right) const {
    ++foo_comparisons;
    return left.val == right.val;
  }

  bool operator()(const Foo &left, const int right) const {
    ++int_comparisons;
    return left.val == right;
  }
};

class foo_hasher {
public:
  size_t operator()(const Foo &x) const {
    ++foo_hashes;
    return static_cast<size_t>(x.val);
  }

  size_t operator()(const int x) const {
    ++int_hashes;
    return static_cast<size_t>(x);
  }
};

typedef seqlock_lib::cuckoo::cuckoohash_map<Foo, bool, foo_hasher, foo_eq> foo_map;

class HeterogeneousCompare : public ::testing::Test {
 protected:
  void SetUp() override {
    int_constructions = 0;
    foo_comparisons = 0;
    int_comparisons = 0;
    foo_hashes = 0;
    int_hashes = 0;
  }
};

TEST_F(HeterogeneousCompare, Insert) {
  {
    foo_map map;
    map.insert(0, true);
  }
  ASSERT_EQ(int_constructions, 1);
  ASSERT_EQ(foo_comparisons, 0);
  ASSERT_EQ(int_comparisons, 0);
  ASSERT_EQ(foo_hashes, 0);
  ASSERT_EQ(int_hashes, 1);
}

TEST_F(HeterogeneousCompare, FooInsert) {
  {
    foo_map map;
    map.insert(Foo(0), true);
    ASSERT_EQ(int_constructions, 1);
  }

  ASSERT_EQ(int_constructions, 1);
  ASSERT_EQ(foo_comparisons, 0);
  ASSERT_EQ(int_comparisons, 0);
  ASSERT_EQ(foo_hashes, 1);
  ASSERT_EQ(int_hashes, 0);
}

TEST_F(HeterogeneousCompare, InsertOrAssign) {
  {
    foo_map map;
    map.insert_or_assign(0, true);
    map.insert_or_assign(0, false);
    ASSERT_FALSE(map.find(0));
  }

  ASSERT_EQ(int_constructions, 1);
  ASSERT_EQ(foo_comparisons, 0);
  ASSERT_EQ(int_comparisons, 2);
  ASSERT_EQ(foo_hashes, 0);
  ASSERT_EQ(int_hashes, 3);
}

TEST_F(HeterogeneousCompare, FooInsertOrAssign) {
  {
    foo_map map;
    map.insert_or_assign(Foo(0), true);
    map.insert_or_assign(Foo(0), false);
    ASSERT_FALSE(map.find(Foo(0)));
  }
  
  ASSERT_EQ(int_constructions, 3);
  ASSERT_EQ(foo_comparisons, 2);
  ASSERT_EQ(int_comparisons, 0);
  ASSERT_EQ(foo_hashes, 3);
  ASSERT_EQ(int_hashes, 0);
}

TEST_F(HeterogeneousCompare, Find) {
  {
    foo_map map;
    map.insert(0, true);
    bool val;
    map.find(0, val);
    ASSERT_TRUE(val);
    ASSERT_TRUE(map.find(0, val));
    ASSERT_FALSE(map.find(1, val));
  }
  
  ASSERT_EQ(int_constructions, 1);
  ASSERT_EQ(foo_comparisons, 0);
  ASSERT_EQ(int_comparisons, 2);
  ASSERT_EQ(foo_hashes, 0);
  ASSERT_EQ(int_hashes, 4);
}

TEST_F(HeterogeneousCompare, FooFind) {
  {
    foo_map map;
    map.insert(0, true);
    bool val;
    map.find(Foo(0), val);
    ASSERT_TRUE(val);
    ASSERT_TRUE(map.find(Foo(0), val));
    ASSERT_FALSE(map.find(Foo(1), val));
  }
  
  ASSERT_EQ(int_constructions, 4);
  ASSERT_EQ(foo_comparisons, 2);
  ASSERT_EQ(int_comparisons, 0);
  ASSERT_EQ(foo_hashes, 3);
  ASSERT_EQ(int_hashes, 1);
}

TEST_F(HeterogeneousCompare, Contains) {
  {
    foo_map map(0);
    map.rehash(2);
    map.insert(0, true);
    ASSERT_TRUE(map.contains(0));
    // Shouldn't do comparison because of different partial key
    ASSERT_FALSE(map.contains(4));
  }
  
  ASSERT_EQ(int_constructions, 1);
  ASSERT_EQ(foo_comparisons, 0);
  ASSERT_EQ(int_comparisons, 1);
  ASSERT_EQ(foo_hashes, 0);
  ASSERT_EQ(int_hashes, 3);
}

TEST_F(HeterogeneousCompare, Erase) {
  {
    foo_map map;
    map.insert(0, true);
    ASSERT_TRUE(map.erase(0));
    ASSERT_FALSE(map.contains(0));
  }
  
  ASSERT_EQ(int_constructions, 1);
  ASSERT_EQ(foo_comparisons, 0);
  ASSERT_EQ(int_comparisons, 1);
  ASSERT_EQ(foo_hashes, 0);
  ASSERT_EQ(int_hashes, 3);
}

TEST_F(HeterogeneousCompare, Update) {
  {
    foo_map map;
    map.insert(0, true);
    ASSERT_TRUE(map.update(0, false));
    ASSERT_FALSE(map.find(0));
  }
  
  ASSERT_EQ(int_constructions, 1);
  ASSERT_EQ(foo_comparisons, 0);
  ASSERT_EQ(int_comparisons, 2);
  ASSERT_EQ(foo_hashes, 0);
  ASSERT_EQ(int_hashes, 3);
}

TEST_F(HeterogeneousCompare, UpdateFn) {
  {
    foo_map map;
    map.insert(0, true);
    ASSERT_TRUE(map.update_fn(0, [](bool &val) { val = !val; }));
    ASSERT_FALSE(map.find(0));
  }
  
  ASSERT_EQ(int_constructions, 1);
  ASSERT_EQ(foo_comparisons, 0);
  ASSERT_EQ(int_comparisons, 2);
  ASSERT_EQ(foo_hashes, 0);
  ASSERT_EQ(int_hashes, 3);
}

TEST_F(HeterogeneousCompare, Upsert) {
  {
    foo_map map(0);
    map.rehash(2);
    auto neg = [](bool &val) { val = !val; };
    map.upsert(0, neg, true);
    map.upsert(0, neg, true);
    // Shouldn't do comparison because of different partial key
    map.upsert(4, neg, false);
    ASSERT_FALSE(map.find(0));
    ASSERT_FALSE(map.find(4));
  }
  
  ASSERT_EQ(int_constructions, 2);
  ASSERT_EQ(foo_comparisons, 0);
  ASSERT_EQ(int_comparisons, 3);
  ASSERT_EQ(foo_hashes, 0);
  ASSERT_EQ(int_hashes, 5);
}

TEST_F(HeterogeneousCompare, UpraseFn) {
  {
    foo_map map(0);
    map.rehash(2);
    auto fn = [](bool &val) {
      val = !val;
      return val;
    };
    ASSERT_TRUE(map.uprase_fn(0, fn, true));
    ASSERT_FALSE(map.uprase_fn(0, fn, true));
    ASSERT_TRUE(map.contains(0));
    ASSERT_FALSE(map.uprase_fn(0, fn, true));
    ASSERT_FALSE(map.contains(0));
  }
  
  ASSERT_EQ(int_constructions, 1);
  ASSERT_EQ(foo_comparisons, 0);
  ASSERT_EQ(int_comparisons, 3);
  ASSERT_EQ(foo_hashes, 0);
  ASSERT_EQ(int_hashes, 5);
}
