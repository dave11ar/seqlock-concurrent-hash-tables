#pragma once

#include "bucket.hpp"
#include "data_storage.hpp"
#include <iostream>

namespace cuckoo_seqlock {

template <class Key, class T, class Allocator, class Partial, size_t SLOT_PER_BUCKET>
class bucket_container : public data_storage<bucket<Key, T, Partial, SLOT_PER_BUCKET>, Allocator>{
public:
  using typed_bucket = bucket<Key, T, Partial, SLOT_PER_BUCKET>;

  using key_type = typename typed_bucket::key_type;
  using mapped_type = typename typed_bucket::mapped_type;
  using partial_t = typename typed_bucket::partial_t;
  using value_type = typename typed_bucket::value_type;

  using reference = value_type&;
  using const_reference = const value_type&;

private:
  using data_storage_base_t = data_storage<typed_bucket, Allocator>;
  using traits_ = typename std::allocator_traits<
      Allocator>::template rebind_traits<value_type>;
  
public:
  using size_type = typename data_storage_base_t::size_type;
  using allocator_type = typename traits_::allocator_type;
  using pointer = typename traits_::pointer;
  using const_pointer = typename traits_::const_pointer;

private:
  void move(bucket_container&& bc) {
    auto it = this->begin();

    for (auto& b : bc) {
      for (size_t slot = 0; slot < SLOT_PER_BUCKET; ++slot) {
        if (b.occupied(slot)) {
          setKV(*it, slot, b.partial(slot), b.movable_key(slot), std::move(b.mapped(slot)));
        }
      }
      ++it;
    }
  }

  void copy(const bucket_container& bc) {
    auto it = this->begin();

    for (const auto& b : bc) {
      for (size_t slot = 0; slot < SLOT_PER_BUCKET; ++slot) {
        if (b.occupied(slot)) {
          setKV(*it, slot, b.partial(slot), b.key(slot), b.mapped(slot));
        }
      }
      ++it;
    }
  }

  // true here means the bucket allocator should be propagated
  void move_assign(bucket_container& other, std::true_type) {
    value_allocator_ = std::move(other.value_allocator_);
    this->data_allocator_ = value_allocator_;

    this->move_pointers(std::move(other));
  }

  void move_assign(bucket_container& other, std::false_type) {
    if (value_allocator_ == other.value_allocator_) {
      this->move_pointers(std::move(other));
    } else {
      this->change_size(other.hashpower());
      move(std::move(other));
    }
  }

public:
  bucket_container(size_type hp, const allocator_type& allocator) 
      : data_storage_base_t(hp, allocator), value_allocator_(allocator) {}

  ~bucket_container() {
    clear_and_deallocate();
  }

  bucket_container(const bucket_container& other)
      : value_allocator_(
          traits_::select_on_container_copy_construction(other.value_allocator_)),
        data_storage_base_t(other.hashpower(), value_allocator_) {
    copy(other);
  }

  bucket_container(const bucket_container& other, const allocator_type& allocator)
      : value_allocator_(allocator), data_storage_base_t(other.hashpower(), value_allocator_) {
    copy(other);
  }

  bucket_container(bucket_container&& other)
    : value_allocator_(std::move(other.value_allocator_)), data_storage_base_t(-1, value_allocator_) {
    this->move_pointers(std::move(other));
  }

  bucket_container(bucket_container&& bc,
                 const allocator_type& allocator)
      : value_allocator_(allocator), data_storage_base_t(-1, value_allocator_) {
    move_assign(bc, std::false_type());
  }

  bucket_container& operator=(const bucket_container& other) {
    clear_and_deallocate();
    copy_allocator(value_allocator_, other.value_allocator_,
                   typename traits_::propagate_on_container_copy_assignment());
    this->data_allocator_ = value_allocator_;

    this->change_size(other.hashpower());
    copy(other);

    return *this;
  }

  bucket_container &operator=(bucket_container&& bc) {
    clear_and_deallocate();
    move_assign(bc, typename traits_::propagate_on_container_move_assignment());
    return *this;
  }

  void swap(bucket_container& other) noexcept {
    swap_allocator(value_allocator_, other.value_allocator_,
        typename traits_::propagate_on_container_swap());

    data_storage_base_t::swap(other);
  }

  // Constructs live data in a bucket
  template <typename K, typename... Args>
  void setKV(typed_bucket& b, size_type slot, partial_t p, K &&k,
             Args &&... args) {
    assert(!b.occupied(slot));
    b.partial(slot) = p;
    traits_::construct(value_allocator_, std::addressof(b.storage_kvpair(slot)),
                       std::piecewise_construct,
                       std::forward_as_tuple(std::forward<K>(k)),
                       std::forward_as_tuple(std::forward<Args>(args)...));
    // This must occur last, to enforce a strong exception guarantee
    b.occupied(slot) = true;
  }
  template <typename K, typename... Args>
  void setKV(size_type ind, size_type slot, partial_t p, K&& k,
             Args&& ...args) {
    setKV((*this)[ind], slot, p, std::forward<K>(k), std::forward<Args>(args)...);
  }

  // Destroys live data in a bucket
  void eraseKV(typed_bucket& b, size_type slot) {
    assert(b.occupied(slot));
    b.occupied(slot) = false;
    traits_::destroy(value_allocator_, std::addressof(b.storage_kvpair(slot)));
  }
  void eraseKV(size_type ind, size_type slot) {
    eraseKV((*this)[ind], slot);
  }

  // Destroys all the live data in the buckets. Does not deallocate the bucket
  // memory.
  void clear() noexcept {
    static_assert(
        std::is_nothrow_destructible<key_type>::value &&
            std::is_nothrow_destructible<mapped_type>::value,
        "bucket_container requires key and value to be nothrow "
        "destructible");
    for (typed_bucket& b : *this) {
      for (size_type slot = 0; slot < SLOT_PER_BUCKET; ++slot) {
        if (b.occupied(slot)) {
          eraseKV(b, slot);
        }
      }
    }
  }

  void clear_and_deallocate() noexcept {
    if (this->is_deallocated()) {
      return;
    }
    // The bucket default constructor is nothrow, so we don't have to
    // worry about dealing with exceptions when constructing all the
    // elements.
    static_assert(std::is_nothrow_destructible<typed_bucket>::value,
                  "bucket_container requires bucket to be nothrow "
                  "destructible");
    clear();
    this->change_size(-1);
  }

  const allocator_type& get_allocator() const {
    return value_allocator_;
  }

private:
  allocator_type value_allocator_;
};

} // cuckoo_seqlock