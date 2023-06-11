#include <cstddef>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <gtest/gtest.h>

#include "../test_utils.hpp"
#include "unit_test_util.hpp"

template <bool PROPAGATE_COPY_ASSIGNMENT = true,
          bool PROPAGATE_MOVE_ASSIGNMENT = true, bool PROPAGATE_SWAP = true>
struct allocator_wrapper {
  template <class T> class stateful_allocator {
  public:
    using value_type = T;
    using propagate_on_container_copy_assignment =
        std::integral_constant<bool, PROPAGATE_COPY_ASSIGNMENT>;
    using propagate_on_container_move_assignment =
        std::integral_constant<bool, PROPAGATE_MOVE_ASSIGNMENT>;
    using propagate_on_container_swap =
        std::integral_constant<bool, PROPAGATE_SWAP>;

    stateful_allocator() : id(0) {}
    stateful_allocator(const size_t &id_) : id(id_) {}
    stateful_allocator(const stateful_allocator &other) : id(other.id) {}
    template <class U>
    stateful_allocator(const stateful_allocator<U> &other) : id(other.id) {}

    stateful_allocator &operator=(const stateful_allocator &a) {
      id = a.id + 1;
      return *this;
    }

    stateful_allocator &operator=(stateful_allocator &&a) {
      id = a.id + 2;
      return *this;
    }

    T *allocate(size_t n) { return std::allocator<T>().allocate(n); }

    void deallocate(T *ptr, size_t n) {
      std::allocator<T>().deallocate(ptr, n);
    }

    stateful_allocator select_on_container_copy_construction() const {
      stateful_allocator copy(*this);
      ++copy.id;
      return copy;
    }

    friend bool operator==(const stateful_allocator& a, 
                           const stateful_allocator& b) {
      return a.id == b.id; 
    }

    friend bool operator!=(const stateful_allocator& a, 
                           const stateful_allocator& b) {
      return !(a == b);
    }

    size_t id;
  };
};

template <class T, bool PCA = true, bool PMA = true>
using stateful_allocator =
    typename allocator_wrapper<PCA, PMA>::template stateful_allocator<T>;

const size_t SLOT_PER_BUCKET = 4;

template <class Alloc>
using TestingContainer =
    seqlock_lib::cuckoo::cuckoo_bucket_container<big_object<10>, int, Alloc, uint8_t,
                               SLOT_PER_BUCKET>;

using value_type = std::pair<const big_object<10>, int>;

TEST(BucketContainer, DefaultConstructor) {
  allocator_wrapper<>::stateful_allocator<value_type> a;
  TestingContainer<decltype(a)> tc(2, a);
  ASSERT_EQ(tc.hashpower(), 2);
  ASSERT_EQ(tc.size(), 4);
  ASSERT_EQ(tc.get_allocator().id, 0);
  for (size_t i = 0; i < tc.size(); ++i) {
    for (size_t j = 0; j < SLOT_PER_BUCKET; ++j) {
      ASSERT_FALSE(tc[i].occupied(j));
    }
  }
}

TEST(BucketContainer, SimpleStatefulAllocator) {
  allocator_wrapper<>::stateful_allocator<value_type> a(10);
  TestingContainer<decltype(a)> tc(2, a);
  ASSERT_EQ(tc.hashpower(), 2);
  ASSERT_EQ(tc.size(), 4);
  ASSERT_EQ(tc.get_allocator().id, 10);
}

TEST(BucketContainer, CopyConstruction) {
  allocator_wrapper<>::stateful_allocator<value_type> a(5);
  TestingContainer<decltype(a)> tc(2, a);
  tc.setKV(0, 0, 2, 10, 5);
  TestingContainer<decltype(a)> tc2(tc);

  ASSERT_TRUE(tc[0].occupied(0));
  ASSERT_EQ(tc[0].partial(0), 2);
  ASSERT_EQ(tc[0].key(0), 10);
  ASSERT_EQ(tc[0].mapped(0), 5);
  ASSERT_EQ(tc.get_allocator().id, 5);

  ASSERT_TRUE(tc2[0].occupied(0));
  ASSERT_EQ(tc2[0].partial(0), 2);
  ASSERT_EQ(tc2[0].key(0), 10);
  ASSERT_EQ(tc2[0].mapped(0), 5);
  ASSERT_EQ(tc2.get_allocator().id, 6);
}

TEST(BucketContainer, MoveConstruction) {
  allocator_wrapper<>::stateful_allocator<value_type> a(5);
  TestingContainer<decltype(a)> tc(2, a);
  tc.setKV(0, 0, 2, 10, 5);
  TestingContainer<decltype(a)> tc2(std::move(tc));

  ASSERT_TRUE(tc2[0].occupied(0));
  ASSERT_EQ(tc2[0].partial(0), 2);
  ASSERT_EQ(tc2[0].key(0), 10);
  ASSERT_EQ(tc2[0].mapped(0), 5);
  ASSERT_EQ(tc2.get_allocator().id, 5);
}

TEST(BucketContainer, CopyAssignmentWithPropagate) {
  allocator_wrapper<true>::stateful_allocator<value_type> a(5);
  TestingContainer<decltype(a)> tc(2, a);
  tc.setKV(0, 0, 2, 10, 5);
  TestingContainer<decltype(a)> tc2(2, a);
  tc2.setKV(1, 0, 2, 10, 5);

  tc2 = tc;
  ASSERT_TRUE(tc2[0].occupied(0));
  ASSERT_EQ(tc2[0].partial(0), 2);
  ASSERT_EQ(tc2[0].key(0), 10);
  ASSERT_EQ(tc2[0].mapped(0), 5);
  ASSERT_FALSE(tc2[1].occupied(0));

  ASSERT_EQ(tc.get_allocator().id, 5);
  ASSERT_EQ(tc2.get_allocator().id, 6);
}

TEST(BucketContainer, CopyAssignmentNoPropagate) {
  allocator_wrapper<false>::stateful_allocator<value_type> a(5);
  TestingContainer<decltype(a)> tc(2, a);
  tc.setKV(0, 0, 2, 10, 5);
  TestingContainer<decltype(a)> tc2(2, a);
  tc2.setKV(1, 0, 2, 10, 5);

  tc2 = tc;
  ASSERT_TRUE(tc2[0].occupied(0));
  ASSERT_EQ(tc2[0].partial(0), 2);
  ASSERT_EQ(tc2[0].key(0), 10);
  ASSERT_EQ(tc2[0].mapped(0), 5);
  ASSERT_FALSE(tc2[1].occupied(0));

  ASSERT_EQ(tc.get_allocator().id, 5);
  ASSERT_EQ(tc2.get_allocator().id, 5);
}

TEST(BucketContainer, MoveAssignmentWithPropagate) {
  allocator_wrapper<>::stateful_allocator<value_type> a(5);
  TestingContainer<decltype(a)> tc(2, a);
  tc.setKV(0, 0, 2, 10, 5);
  TestingContainer<decltype(a)> tc2(2, a);
  tc2.setKV(1, 0, 2, 10, 5);

  tc2 = std::move(tc);
  ASSERT_TRUE(tc2[0].occupied(0));
  ASSERT_EQ(tc2[0].partial(0), 2);
  ASSERT_EQ(tc2[0].key(0), 10);
  ASSERT_EQ(tc2[0].mapped(0), 5);
  ASSERT_FALSE(tc2[1].occupied(0));
  ASSERT_EQ(tc2.get_allocator().id, 7);
}

TEST(BucketContainer, MoveAssignmentNoPropagateEqual) {
  allocator_wrapper<true, false>::stateful_allocator<value_type> a(5);
  TestingContainer<decltype(a)> tc(2, a);
  tc.setKV(0, 0, 2, 10, 5);
  TestingContainer<decltype(a)> tc2(2, a);
  tc2.setKV(1, 0, 2, 10, 5);

  tc2 = std::move(tc);
  ASSERT_TRUE(tc2[0].occupied(0));
  ASSERT_EQ(tc2[0].partial(0), 2);
  ASSERT_EQ(tc2[0].key(0), 10);
  ASSERT_EQ(tc2[0].mapped(0), 5);
  ASSERT_FALSE(tc2[1].occupied(0));
  ASSERT_EQ(tc2.get_allocator().id, 5);
}

TEST(BucketContainer, MoveAssignmentNoPropagateUnequal) {
  allocator_wrapper<true, false>::stateful_allocator<value_type> a(5);
  TestingContainer<decltype(a)> tc(2, a);
  tc.setKV(0, 0, 2, 10, 5);
  allocator_wrapper<true, false>::stateful_allocator<value_type> a2(4);
  TestingContainer<decltype(a)> tc2(2, a2);
  tc2.setKV(1, 0, 2, 10, 5);

  tc2 = std::move(tc);
  ASSERT_FALSE(tc2[1].occupied(0));
  ASSERT_TRUE(tc2[0].occupied(0));
  ASSERT_EQ(tc2[0].partial(0), 2);
  ASSERT_EQ(tc2[0].key(0), 10);
  ASSERT_EQ(tc2[0].mapped(0), 5);
  ASSERT_FALSE(tc2[1].occupied(0));
  ASSERT_EQ(tc2.get_allocator().id, 4);

  ASSERT_TRUE(tc[0].occupied(0));
  ASSERT_EQ(tc[0].partial(0), 2);
}

TEST(BucketContainer, SwapNoPropagate) {
  allocator_wrapper<true, true, false>::stateful_allocator<value_type> a(5);
  TestingContainer<decltype(a)> tc(2, a);
  tc.setKV(0, 0, 2, 10, 5);
  TestingContainer<decltype(a)> tc2(2, a);
  tc2.setKV(1, 0, 2, 10, 5);

  tc.swap(tc2);

  ASSERT_TRUE(tc[1].occupied(0));
  ASSERT_EQ(tc[1].partial(0), 2);
  ASSERT_EQ(tc[1].key(0), 10);
  ASSERT_EQ(tc[1].mapped(0), 5);
  ASSERT_EQ(tc.get_allocator().id, 5);

  ASSERT_TRUE(tc2[0].occupied(0));
  ASSERT_EQ(tc2[0].partial(0), 2);
  ASSERT_EQ(tc2[0].key(0), 10);
  ASSERT_EQ(tc2[0].mapped(0), 5);
  ASSERT_EQ(tc2.get_allocator().id, 5);
}

TEST(BucketContainer, SwapPropagate) {
  allocator_wrapper<true, true, true>::stateful_allocator<value_type> a(5);
  TestingContainer<decltype(a)> tc(2, a);
  tc.setKV(0, 0, 2, 10, 5);
  TestingContainer<decltype(a)> tc2(2, a);
  tc2.setKV(1, 0, 2, 10, 5);

  tc.swap(tc2);

  ASSERT_TRUE(tc[1].occupied(0));
  ASSERT_EQ(tc[1].partial(0), 2);
  ASSERT_EQ(tc[1].key(0), 10);
  ASSERT_EQ(tc[1].mapped(0), 5);
  ASSERT_EQ(tc.get_allocator().id, 7);

  ASSERT_TRUE(tc2[0].occupied(0));
  ASSERT_EQ(tc2[0].partial(0), 2);
  ASSERT_EQ(tc2[0].key(0), 10);
  ASSERT_EQ(tc2[0].mapped(0), 5);
  ASSERT_EQ(tc2.get_allocator().id, 7);
}
