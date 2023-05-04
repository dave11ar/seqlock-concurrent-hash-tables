#include <memory>
#include <sstream>
#include <stdexcept>
#include <type_traits>

#include <gtest/gtest.h>

#include <seqlock.hpp>
#include "unit_test_util.hpp"

TEST(LockedTable, Typedefs) {
  using Tbl = IntIntTable;
  using Ltbl = Tbl::locked_table;
  const bool key_type = std::is_same<Tbl::key_type, Ltbl::key_type>::value;
  const bool mapped_type =
      std::is_same<Tbl::mapped_type, Ltbl::mapped_type>::value;
  const bool value_type =
      std::is_same<Tbl::value_type, Ltbl::value_type>::value;
  const bool size_type = std::is_same<Tbl::size_type, Ltbl::size_type>::value;
  const bool difference_type =
      std::is_same<Tbl::difference_type, Ltbl::difference_type>::value;
  const bool hasher = std::is_same<Tbl::hasher, Ltbl::hasher>::value;
  const bool key_equal = std::is_same<Tbl::key_equal, Ltbl::key_equal>::value;
  const bool allocator_type =
      std::is_same<Tbl::allocator_type, Ltbl::allocator_type>::value;
  const bool reference = std::is_same<Tbl::reference, Ltbl::reference>::value;
  const bool const_reference =
      std::is_same<Tbl::const_reference, Ltbl::const_reference>::value;
  const bool pointer = std::is_same<Tbl::pointer, Ltbl::pointer>::value;
  const bool const_pointer =
      std::is_same<Tbl::const_pointer, Ltbl::const_pointer>::value;
  ASSERT_TRUE(key_type);
  ASSERT_TRUE(mapped_type);
  ASSERT_TRUE(value_type);
  ASSERT_TRUE(size_type);
  ASSERT_TRUE(difference_type);
  ASSERT_TRUE(hasher);
  ASSERT_TRUE(key_equal);
  ASSERT_TRUE(allocator_type);
  ASSERT_TRUE(reference);
  ASSERT_TRUE(const_reference);
  ASSERT_TRUE(pointer);
  ASSERT_TRUE(const_pointer);
}

TEST(LockedTable, MoveConstructor) {
  IntIntTable tbl;
  auto lt = tbl.lock_table();
  auto lt2(std::move(lt));
  ASSERT_FALSE(lt.is_active());
  ASSERT_TRUE(lt2.is_active());
}

TEST(LockedTable, MoveAssignment) {
  IntIntTable tbl;
  auto lt = tbl.lock_table();
  auto lt2 = std::move(lt);
  ASSERT_FALSE(lt.is_active());
  ASSERT_TRUE(lt2.is_active());
}

TEST(LockedTable, IteratorsCompareAfterTableIsMoved) {
  IntIntTable tbl;
  auto lt1 = tbl.lock_table();
  auto it1 = lt1.begin();
  auto it2 = lt1.begin();
  ASSERT_EQ(it1, it2);
  auto lt2(std::move(lt1));
  ASSERT_EQ(it1, it2);
}

TEST(LockedTable, Unlock) {
  IntIntTable tbl;
  tbl.insert(10, 10);
  auto lt = tbl.lock_table();
  lt.unlock();
  ASSERT_FALSE(lt.is_active());
}

TEST(LockedTable, Info) {
  IntIntTable tbl;
  tbl.insert(10, 10);
  auto lt = tbl.lock_table();
  ASSERT_TRUE(lt.is_active());

  // We should still be able to call table info operations on the
  // cuckoohash_map instance, because they shouldn't take locks.

  ASSERT_EQ(lt.slot_per_bucket(), tbl.slot_per_bucket());
  ASSERT_EQ(lt.get_allocator(), tbl.get_allocator());
  ASSERT_EQ(lt.hashpower(), tbl.hashpower());
  ASSERT_EQ(lt.bucket_count(), tbl.bucket_count());
  ASSERT_EQ(lt.empty(), tbl.empty());
  ASSERT_EQ(lt.size(), tbl.size());
  ASSERT_EQ(lt.capacity(), tbl.capacity());
  ASSERT_EQ(lt.load_factor(), tbl.load_factor());
  ASSERT_THROW(lt.minimum_load_factor(1.01), std::invalid_argument);
  lt.minimum_load_factor(lt.minimum_load_factor() * 2);
  lt.rehash(5);
  ASSERT_THROW(lt.maximum_hashpower(lt.hashpower() - 1),
               std::invalid_argument);
  lt.maximum_hashpower(lt.hashpower() + 1);
  ASSERT_EQ(lt.maximum_hashpower(), tbl.maximum_hashpower());
}

TEST(LockedTable, Clear) {
  IntIntTable tbl;
  tbl.insert(10, 10);
  auto lt = tbl.lock_table();
  ASSERT_EQ(lt.size(), 1);
  lt.clear();
  ASSERT_EQ(lt.size(), 0);
  lt.clear();
  ASSERT_EQ(lt.size(), 0);
}

TEST(LockedTable, InsertDuplicate) {
  IntIntTable tbl;
  tbl.insert(10, 10);
  {
    auto lt = tbl.lock_table();
    auto result = lt.insert(10, 20);
    ASSERT_EQ(result.first->first, 10);
    ASSERT_EQ(result.first->second, 10);
    ASSERT_FALSE(result.second);
    result.first->second = 50;
  }
  ASSERT_EQ(tbl.find(10), 50);
}

TEST(LockedTable, InsertNewKey) {
  IntIntTable tbl;
  tbl.insert(10, 10);
  {
    auto lt = tbl.lock_table();
    auto result = lt.insert(20, 20);
    ASSERT_EQ(result.first->first, 20);
    ASSERT_EQ(result.first->second, 20);
    ASSERT_TRUE(result.second);
    result.first->second = 50;
  }
  ASSERT_EQ(tbl.find(10), 10);
  ASSERT_EQ(tbl.find(20), 50);
}

TEST(LockedTable, SuccessfulInsertLifetime) {
  UniquePtrTable<int> tbl;
  auto lt = tbl.lock_table();
  std::unique_ptr<int> key(new int(20));
  std::unique_ptr<int> value(new int(20));
  auto result = lt.insert(std::move(key), std::move(value));
  ASSERT_EQ(*result.first->first, 20);
  ASSERT_EQ(*result.first->second, 20);
  ASSERT_TRUE(result.second);
  ASSERT_FALSE(static_cast<bool>(key));
  ASSERT_FALSE(static_cast<bool>(value));
}

TEST(LockedTable, UnsuccessfulInsertLifetime) {
  UniquePtrTable<int> tbl;
  tbl.insert(new int(20), new int(20));
  auto lt = tbl.lock_table();
  std::unique_ptr<int> key(new int(20));
  std::unique_ptr<int> value(new int(30));
  auto result = lt.insert(std::move(key), std::move(value));
  ASSERT_EQ(*result.first->first, 20);
  ASSERT_EQ(*result.first->second, 20);
  ASSERT_FALSE(result.second);
  ASSERT_TRUE(static_cast<bool>(key));
  ASSERT_TRUE(static_cast<bool>(value));
}

TEST(LockedTable, SimpleErase) {
  IntIntTable tbl;
  for (int i = 0; i < 5; ++i) {
    tbl.insert(i, i);
  }
  using lt_t = IntIntTable::locked_table;

  auto lt = tbl.lock_table();
  lt_t::const_iterator const_it;
  const_it = lt.find(0);
  ASSERT_NE(const_it, lt.end());
  lt_t::const_iterator const_next = const_it;
  ++const_next;
  ASSERT_EQ(static_cast<lt_t::const_iterator>(lt.erase(const_it)), const_next);
  ASSERT_EQ(lt.size(), 4);

  lt_t::iterator it;
  it = lt.find(1);
  lt_t::iterator next = it;
  ++next;
  ASSERT_EQ(lt.erase(static_cast<lt_t::const_iterator>(it)), next);
  ASSERT_EQ(lt.size(), 3);

  ASSERT_EQ(lt.erase(2), 1);
  ASSERT_EQ(lt.size(), 2);
}

TEST(LockedTable, EraseDoesntRuinThisIterator) {
  IntIntTable tbl;
  for (int i = 0; i < 5; ++i) {
    tbl.insert(i, i);
  }
  using lt_t = IntIntTable::locked_table;

  auto lt = tbl.lock_table();
  auto it = lt.begin();
  auto next = it;
  ++next;
  ASSERT_EQ(lt.erase(it),  next);
  ++it;
  ASSERT_GT(it->first, 0);
  ASSERT_LT(it->first, 5);
  ASSERT_GT(it->second, 0);
  ASSERT_LT(it->second, 5);
}


TEST(LockedTable, EraseDoesntRuinOtherIterators) {
  IntIntTable tbl;
  for (int i = 0; i < 5; ++i) {
    tbl.insert(i, i);
  }
  using lt_t = IntIntTable::locked_table;

  auto lt = tbl.lock_table();
  auto it0 = lt.find(0);
  auto it1 = lt.find(1);
  auto it2 = lt.find(2);
  auto it3 = lt.find(3);
  auto it4 = lt.find(4);
  auto next = it2;
  ++next;
  ASSERT_EQ(lt.erase(it2), next);
  ASSERT_EQ(it0->first, 0);
  ASSERT_EQ(it0->second, 0);
  ASSERT_EQ(it1->first, 1);
  ASSERT_EQ(it1->second, 1);
  ASSERT_EQ(it3->first, 3);
  ASSERT_EQ(it3->second, 3);
  ASSERT_EQ(it4->first, 4);
  ASSERT_EQ(it4->second, 4);
}

TEST(LockedTable, Find) {
  IntIntTable tbl;
  using lt_t = IntIntTable::locked_table;
  auto lt = tbl.lock_table();
  for (int i = 0; i < 10; ++i) {
    ASSERT_TRUE(lt.insert(i, i).second);
  }
  bool found_begin_elem = false;
  bool found_last_elem = false;
  for (int i = 0; i < 10; ++i) {
    lt_t::iterator it = lt.find(i);
    lt_t::const_iterator const_it = lt.find(i);
    ASSERT_NE(it, lt.end());
    ASSERT_EQ(it->first, i);
    ASSERT_EQ(it->second, i);
    ASSERT_NE(const_it, lt.end());
    ASSERT_EQ(const_it->first, i);
    ASSERT_EQ(const_it->second, i);
    it->second++;
    if (it == lt.begin()) {
      found_begin_elem = true;
    }
    if (++it == lt.end()) {
      found_last_elem = true;
    }
  }
  ASSERT_TRUE(found_begin_elem);
  ASSERT_TRUE(found_last_elem);
  for (int i = 0; i < 10; ++i) {
    lt_t::iterator it = lt.find(i);
    ASSERT_EQ(it->first, i);
    ASSERT_EQ(it->second, i + 1);
  }
}

TEST(LockedTable, At) {
  IntIntTable tbl;
  auto lt = tbl.lock_table();
  for (int i = 0; i < 10; ++i) {
    ASSERT_TRUE(lt.insert(i, i).second);
  }
  for (int i = 0; i < 10; ++i) {
    int &val = lt.at(i);
    const int &const_val =
        const_cast<const IntIntTable::locked_table &>(lt).at(i);
    ASSERT_EQ(val, i);
    ASSERT_EQ(const_val, i);
    ++val;
  }
  for (int i = 0; i < 10; ++i) {
    ASSERT_EQ(lt.at(i), i + 1);
  }
  ASSERT_THROW(lt.at(11), std::out_of_range);
}

TEST(LockedTable, OperatorBrackets) {
  IntIntTable tbl;
  auto lt = tbl.lock_table();
  for (int i = 0; i < 10; ++i) {
    ASSERT_TRUE(lt.insert(i, i).second);
  }
  for (int i = 0; i < 10; ++i) {
    int &val = lt[i];
    ASSERT_EQ(val, i);
    ++val;
  }
  for (int i = 0; i < 10; ++i) {
    ASSERT_EQ(lt[i], i + 1);
  }
  ASSERT_EQ(lt[11], 0);
  ASSERT_EQ(lt.at(11), 0);
}

TEST(LockedTable, Count) {
  IntIntTable tbl;
  auto lt = tbl.lock_table();
  for (int i = 0; i < 10; ++i) {
    ASSERT_TRUE(lt.insert(i, i).second);
  }
  for (int i = 0; i < 10; ++i) {
    ASSERT_EQ(lt.count(i), 1);
  }
  ASSERT_EQ(lt.count(11), 0);
}

TEST(LockedTable, EqualRange) {
  IntIntTable tbl;
  using lt_t = IntIntTable::locked_table;
  auto lt = tbl.lock_table();
  for (int i = 0; i < 10; ++i) {
    ASSERT_TRUE(lt.insert(i, i).second);
  }
  for (int i = 0; i < 10; ++i) {
    std::pair<lt_t::iterator, lt_t::iterator> it_range = lt.equal_range(i);
    ASSERT_EQ(it_range.first->first, i);
    ASSERT_EQ(++it_range.first, it_range.second);
    std::pair<lt_t::const_iterator, lt_t::const_iterator> const_it_range =
        lt.equal_range(i);
    ASSERT_EQ(const_it_range.first->first, i);
    ASSERT_EQ(++const_it_range.first, const_it_range.second);
  }
  auto it_range = lt.equal_range(11);
  ASSERT_EQ(it_range.first, lt.end());
  ASSERT_EQ(it_range.second, lt.end());
}

TEST(LockedTable, Rehash) {
  IntIntTable tbl(10);
  auto lt = tbl.lock_table();
  ASSERT_EQ(lt.hashpower(), 2);
  lt.rehash(1);
  ASSERT_EQ(lt.hashpower(), 1);
  lt.rehash(10);
  ASSERT_EQ(lt.hashpower(), 10);
}

TEST(LockedTable, Reserve) {
  IntIntTable tbl(10);
  auto lt = tbl.lock_table();
  ASSERT_EQ(lt.hashpower(), 2);
  lt.reserve(1);
  ASSERT_EQ(lt.hashpower(), 0);
  lt.reserve(4096);
  ASSERT_EQ(lt.hashpower(), 10);
}

TEST(LockedTable, Equality) {
  IntIntTable tbl1(40);
  auto lt1 = tbl1.lock_table();
  for (int i = 0; i < 10; ++i) {
    lt1.insert(i, i);
  }

  IntIntTable tbl2(30);
  auto lt2 = tbl2.lock_table();
  for (int i = 0; i < 10; ++i) {
    lt2.insert(i, i);
  }

  IntIntTable tbl3(30);
  auto lt3 = tbl3.lock_table();
  for (int i = 0; i < 10; ++i) {
    lt3.insert(i, i + 1);
  }

  IntIntTable tbl4(40);
  auto lt4 = tbl4.lock_table();
  for (int i = 0; i < 10; ++i) {
    lt4.insert(i + 1, i);
  }

  ASSERT_TRUE(lt1 == lt2);
  ASSERT_FALSE(lt2 != lt1);

  ASSERT_TRUE(lt1 != lt3);
  ASSERT_FALSE(lt3 == lt1);
  ASSERT_FALSE(lt2 == lt3);
  ASSERT_TRUE(lt3 != lt2);

  ASSERT_TRUE(lt1 != lt4);
  ASSERT_TRUE(lt4 != lt1);
  ASSERT_FALSE(lt3 == lt4);
  ASSERT_FALSE(lt4 == lt3);
}

template <typename Table> void check_all_locks_taken(Table &tbl) {
  auto &locks = seqlock_lib::cuckoo::UnitTestInternalAccess::get_locks(tbl);
  for (auto &lock : locks) {
    ASSERT_FALSE(lock.try_lock());
  }
}

TEST(LockedTable, HoldsLocksAfterResize) {
  IntIntTable tbl(4);
  auto lt = tbl.lock_table();
  check_all_locks_taken(tbl);

  // After a cuckoo_fast_double, all locks are still taken
  for (int i = 0; i < 5; ++i) {
    lt.insert(i, i);
  }
  check_all_locks_taken(tbl);

  // After a cuckoo_simple_expand, all locks are still taken
  lt.rehash(10);
  check_all_locks_taken(tbl);
}
