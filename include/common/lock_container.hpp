#pragma once

#include "data_storage.hpp"

namespace seqlock_lib {

template <typename Lock, typename Allocator>
class lock_container : public data_storage<Lock, Allocator> {
  using data_storage_base_t = data_storage<Lock, Allocator>;

  void copy(const lock_container& other) {
    auto it = this->begin();

    for (const Lock& lock : other) {
      it->set_migrated(Lock::is_migrated(lock.get_epoch()));
      it->elem_counter() = lock.elem_counter();

      ++it;
    }
  }

  // true here means the allocator should be propagated
  void move_assign(lock_container& other, std::true_type) {
    this->data_allocator_ = std::move(other.data_allocator_);
    this->move_pointers(std::move(other));
  }

  void move_assign(lock_container& other, std::false_type) {
    if (this->data_allocator_ == other.data_allocator_) {
      this->move_pointers(std::move(other));
    } else {
      this->change_size(other.hashpower());
      copy(other);
    }
  }

public:
  lock_container(size_t hp, const Allocator& allocator) 
      : data_storage_base_t(hp, allocator) {}

  ~lock_container() = default;

  lock_container(const lock_container& other) 
      : data_storage_base_t(other.hashpower(), other.data_allocator_) {
    copy(other);
  };
  lock_container(const lock_container& other, const Allocator& allocator) 
      : data_storage_base_t(other.hashpower(), allocator) {
    copy(other);
  }

  lock_container(lock_container&& other) 
      : data_storage_base_t(-1, other.data_allocator_) {
    this->move_pointers(std::move(other));
  };
  lock_container(lock_container&& other, const Allocator& allocator) 
      : data_storage_base_t(-1, allocator) {
    move_assign(other, std::false_type());
  };

  lock_container& operator=(const lock_container& other) {
    copy_allocator(this->data_allocator_, other.data_allocator_,
                   typename data_storage_base_t::traits::propagate_on_container_copy_assignment());
    this->change_size(other.hashpower());
    copy(other);
    return *this;
  }
  lock_container& operator=(lock_container&& other) {
    this->change_size(-1);
    move_assign(other, typename data_storage_base_t::traits::propagate_on_container_move_assignment());
    return *this;
  }
};

} // namespace seqlock_lib
