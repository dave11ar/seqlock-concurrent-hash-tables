#include <algorithm>
#include <chrono>
#include <stdexcept>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>

#include <gtest/gtest.h>

#include "unit_test_util.hpp"

TEST(Iterator, IteratorTypes) {
  using Ltbl = IntIntTable::locked_table;
  using It = Ltbl::iterator;
  using ConstIt = Ltbl::const_iterator;

  const bool it_difference_type =
      std::is_same<Ltbl::difference_type, It::difference_type>::value;
  const bool it_value_type =
      std::is_same<Ltbl::value_type, It::value_type>::value;
  const bool it_pointer = std::is_same<Ltbl::pointer, It::pointer>::value;
  const bool it_reference = std::is_same<Ltbl::reference, It::reference>::value;
  const bool it_iterator_category =
      std::is_same<std::bidirectional_iterator_tag,
                   It::iterator_category>::value;

  const bool const_it_difference_type =
      std::is_same<Ltbl::difference_type, ConstIt::difference_type>::value;
  const bool const_it_value_type =
      std::is_same<Ltbl::value_type, ConstIt::value_type>::value;
  const bool const_it_reference =
      std::is_same<Ltbl::const_reference, ConstIt::reference>::value;
  const bool const_it_pointer =
      std::is_same<Ltbl::const_pointer, ConstIt::pointer>::value;
  const bool const_it_iterator_category =
      std::is_same<std::bidirectional_iterator_tag,
                   ConstIt::iterator_category>::value;

  ASSERT_TRUE(it_difference_type);
  ASSERT_TRUE(it_value_type);
  ASSERT_TRUE(it_pointer);
  ASSERT_TRUE(it_reference);
  ASSERT_TRUE(it_iterator_category);

  ASSERT_TRUE(const_it_difference_type);
  ASSERT_TRUE(const_it_value_type);
  ASSERT_TRUE(const_it_pointer);
  ASSERT_TRUE(const_it_reference);
  ASSERT_TRUE(const_it_iterator_category);
}

TEST(Iterator, EmptyTableIteration) {
  IntIntTable table;
  {
    auto lt = table.lock_table();
    ASSERT_EQ(lt.begin(), lt.begin());
    ASSERT_EQ(lt.begin(), lt.end());

    ASSERT_EQ(lt.cbegin(), lt.begin());
    ASSERT_EQ(lt.begin(), lt.end());

    ASSERT_EQ(lt.cbegin(), lt.begin());
    ASSERT_EQ(lt.cend(), lt.end());
  }
}

class IteratorWalkthrough : public ::testing::Test {
 public:
  IntIntTable table;

 protected:
  void SetUp() override {
    for (int i = 0; i < 10; ++i) {
      table.insert(i, i);
    }
  }
};

TEST_F(IteratorWalkthrough, ForwardPostfixWalkthrough) {
  auto lt = table.lock_table();
  auto it = lt.cbegin();
  for (size_t i = 0; i < table.size(); ++i) {
    ASSERT_EQ((*it).first, (*it).second);
    ASSERT_EQ(it->first, it->second);
    auto old_it = it;
    ASSERT_EQ(old_it, it++);
  }
  ASSERT_EQ(it, lt.end());
}

TEST_F(IteratorWalkthrough, ForwardPrefixWalkthroughh) {
  auto lt = table.lock_table();
  auto it = lt.cbegin();
  for (size_t i = 0; i < table.size(); ++i) {
    ASSERT_EQ((*it).first, (*it).second);
    ASSERT_EQ(it->first, it->second);
    ++it;
  }
  ASSERT_EQ(it, lt.end());
}

TEST_F(IteratorWalkthrough, BackwardsPostfixWalkthrough) {
  auto lt = table.lock_table();
  auto it = lt.cend();
  for (size_t i = 0; i < table.size(); ++i) {
    auto old_it = it;
    ASSERT_EQ(old_it, it--);
    ASSERT_EQ((*it).first, (*it).second);
    ASSERT_EQ(it->first, it->second);
  }
  ASSERT_EQ(it, lt.begin());
}

TEST_F(IteratorWalkthrough, BackwardsPrefixWalkthrough) {
  auto lt = table.lock_table();
  auto it = lt.cend();
  for (size_t i = 0; i < table.size(); ++i) {
    --it;
    ASSERT_EQ((*it).first, (*it).second);
    ASSERT_EQ(it->first, it->second);
  }
  ASSERT_EQ(it, lt.begin());
}

TEST_F(IteratorWalkthrough, WalkthroughWorksAfterMove) {
  auto lt = table.lock_table();
  auto it = lt.cend();
  auto lt2 = std::move(lt);
  for (size_t i = 0; i < table.size(); ++i) {
    --it;
    ASSERT_EQ((*it).first, (*it).second);
    ASSERT_EQ(it->first, it->second);
  }
  ASSERT_EQ(it, lt2.begin());
}

TEST(Iterator, IteratorModification) {
  IntIntTable table;
  for (int i = 0; i < 10; ++i) {
    table.insert(i, i);
  }

  auto lt = table.lock_table();
  for (auto it = lt.begin(); it != lt.end(); ++it) {
    it->second = it->second + 1;
  }

  auto it = lt.cbegin();
  for (size_t i = 0; i < table.size(); ++i) {
    ASSERT_EQ(it->first, it->second - 1);
    ++it;
  }
  ASSERT_EQ(it, lt.end());
}

TEST(Iterator, LockTableBlocksInserts) {
  IntIntTable table;
  auto lt = table.lock_table();
  std::thread thread([&table]() {
    for (int i = 0; i < 10; ++i) {
      table.insert(i, i);
    }
  });
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  ASSERT_EQ(table.size(), 0);
  lt.unlock();
  thread.join();

  ASSERT_EQ(table.size(), 10);
}

TEST(Iterator, CastIteratorToConstIterator) {
  IntIntTable table;
  for (int i = 0; i < 10; ++i) {
    table.insert(i, i);
  }
  auto lt = table.lock_table();
  for (IntIntTable::locked_table::iterator it = lt.begin(); it != lt.end();
       ++it) {
    ASSERT_EQ(it->first, it->second);
    it->second++;
    IntIntTable::locked_table::const_iterator const_it = it;
    ASSERT_EQ(it->first + 1, it->second);
  }
}
