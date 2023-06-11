#include <stdexcept>

#include <gtest/gtest.h>

#include "unit_test_util.hpp"

#include <cuckoo/cuckoohash_map.hpp>

using seqlock_lib::cuckoo::UnitTestInternalAccess;

void maybeThrow(bool throwException) {
  if (throwException) {
    throw std::runtime_error("exception");
  }
}

bool constructorThrow, hashThrow, equalityThrow;

class ExceptionInt {
public:
  ExceptionInt() {
    maybeThrow(constructorThrow);
    val = 0;
  }

  ExceptionInt(size_t x) {
    maybeThrow(constructorThrow);
    val = x;
  }

  operator size_t() const { return val; }

private:
  size_t val;
};

namespace std {
template <> struct hash<ExceptionInt> {
  size_t operator()(const ExceptionInt &x) const {
    maybeThrow(hashThrow);
    return x;
  }
};

template <> struct equal_to<ExceptionInt> {
  bool operator()(const ExceptionInt &lhs, const ExceptionInt &rhs) const {
    maybeThrow(equalityThrow);
    return static_cast<size_t>(lhs) == static_cast<size_t>(rhs);
  }
};
}

typedef seqlock_lib::cuckoo::cuckoohash_map<ExceptionInt, size_t, std::hash<ExceptionInt>,
                       std::equal_to<ExceptionInt>>
    exceptionTable;

void checkIterTable(exceptionTable &tbl, size_t expectedSize) {
  auto lockedTable = tbl.lock_table();
  size_t actualSize = 0;
  for (auto it = lockedTable.begin(); it != lockedTable.end(); ++it) {
    ++actualSize;
  }
  ASSERT_EQ(actualSize, expectedSize);
}

class UserExceptions : public ::testing::Test {
 protected:
  void SetUp() override {
    constructorThrow = hashThrow = equalityThrow = false;
  }
};


TEST_F(UserExceptions, FindContains) {
  constructorThrow = hashThrow = equalityThrow = false;
  exceptionTable tbl;
  tbl.insert(1, 1);
  tbl.insert(2, 2);
  tbl.insert(3, 3);
  hashThrow = true;
  ASSERT_THROW(tbl.find(3), std::runtime_error);
  ASSERT_THROW(tbl.contains(3), std::runtime_error);
  hashThrow = false;
  equalityThrow = true;
  ASSERT_THROW(tbl.find(3), std::runtime_error);
  ASSERT_THROW(tbl.contains(3), std::runtime_error);
  equalityThrow = false;
  ASSERT_EQ(tbl.find(3), 3);
  ASSERT_TRUE(tbl.contains(3));
  checkIterTable(tbl, 3);
}

TEST_F(UserExceptions, Insert) {
  exceptionTable tbl;
  constructorThrow = true;
  ASSERT_THROW(tbl.insert(100, 100), std::runtime_error);
  constructorThrow = false;
  ASSERT_TRUE(tbl.insert(100, 100));
  checkIterTable(tbl, 1);
}

TEST_F(UserExceptions, Erase) {
  exceptionTable tbl;
  for (int i = 0; i < 10; ++i) {
    tbl.insert(i, i);
  }
  hashThrow = true;
  ASSERT_THROW(tbl.erase(5), std::runtime_error);
  hashThrow = false;
  equalityThrow = true;
  ASSERT_THROW(tbl.erase(5), std::runtime_error);
  equalityThrow = false;
  ASSERT_TRUE(tbl.erase(5));
  checkIterTable(tbl, 9);
}

TEST_F(UserExceptions, Update) {
  exceptionTable tbl;
  tbl.insert(9, 9);
  tbl.insert(10, 10);
  hashThrow = true;
  ASSERT_THROW(tbl.update(9, 10), std::runtime_error);
  hashThrow = false;
  equalityThrow = true;
  ASSERT_THROW(tbl.update(9, 10), std::runtime_error);
  equalityThrow = false;
  ASSERT_TRUE(tbl.update(9, 10));
  checkIterTable(tbl, 2);
}


TEST_F(UserExceptions, UpdateFn) {
  exceptionTable tbl;
  tbl.insert(9, 9);
  tbl.insert(10, 10);
  auto updater = [](size_t &val) { val++; };
  hashThrow = true;
  ASSERT_THROW(tbl.update_fn(9, updater), std::runtime_error);
  hashThrow = false;
  equalityThrow = true;
  ASSERT_THROW(tbl.update_fn(9, updater), std::runtime_error);
  equalityThrow = false;
  ASSERT_TRUE(tbl.update_fn(9, updater));
  checkIterTable(tbl, 2);
}

TEST_F(UserExceptions, Upsert) {
  exceptionTable tbl;
  tbl.insert(9, 9);
  auto updater = [](size_t &val) { val++; };
  hashThrow = true;
  ASSERT_THROW(tbl.upsert(9, updater, 10), std::runtime_error);
  hashThrow = false;
  equalityThrow = true;
  ASSERT_THROW(tbl.upsert(9, updater, 10), std::runtime_error);
  equalityThrow = false;
  tbl.upsert(9, updater, 10);
  constructorThrow = true;
  ASSERT_THROW(tbl.upsert(10, updater, 10), std::runtime_error);
  constructorThrow = false;
  tbl.upsert(10, updater, 10);
  checkIterTable(tbl, 2);
}

TEST_F(UserExceptions, InsertResize) {
  exceptionTable tbl(1000);
  ASSERT_TRUE(tbl.rehash(1));
  // Fill up the entire table
  for (size_t i = 0; i < exceptionTable::slot_per_bucket() * 2; ++i) {
    tbl.insert(i * 2, 0);
  }
  ASSERT_TRUE(tbl.insert((exceptionTable::slot_per_bucket() * 2) * 2, 0));
  checkIterTable(tbl, exceptionTable::slot_per_bucket() * 2 + 1);
}
