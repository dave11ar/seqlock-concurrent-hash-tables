#pragma once

#include "atomic_buffer.hpp"
#include "bucket.hpp"
#include "data_storage.hpp"

namespace seqlock_lib {

template <typename Derived, typename Bucket, typename Allocator>
class bucket_container : public data_storage<Bucket, Allocator>{
public:
  using typed_bucket = Bucket;

  using key_type = typename typed_bucket::key_type;
  using mapped_type = typename typed_bucket::mapped_type;
  using value_type = typename typed_bucket::value_type;

  using reference = value_type&;
  using const_reference = const value_type&;

private:
  using data_storage_base_t = data_storage<typed_bucket, Allocator>;

protected:
  using traits = typename std::allocator_traits<
      Allocator>::template rebind_traits<value_type>;
  
public:
  using size_type = typename data_storage_base_t::size_type;
  using allocator_type = typename traits::allocator_type;
  using pointer = typename traits::pointer;
  using const_pointer = typename traits::const_pointer;

private:
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
      copy(other);
    }
  }

  void copy(const bucket_container& bc) {
    auto it = this->begin();

    for (const auto& b : bc) {
      for (uint16_t slot = 0; slot < typed_bucket::SLOT_PER_BUCKET; ++slot) {
        if (b.occupied(slot)) {
          Derived::copy_bucket_slot(this, slot, *it, b);
        }
      }
      ++it;
    }
  }

public:
  bucket_container(size_type hp, const allocator_type& allocator) 
      : data_storage_base_t(hp, allocator), value_allocator_(allocator) {}

  ~bucket_container() {
    deallocate();
  }

  bucket_container(const bucket_container& other)
      : value_allocator_(
          traits::select_on_container_copy_construction(other.value_allocator_)),
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
    deallocate();
    copy_allocator(value_allocator_, other.value_allocator_,
                   typename traits::propagate_on_container_copy_assignment());
    this->data_allocator_ = value_allocator_;

    this->change_size(other.hashpower());
    copy(other);

    return *this;
  }

  bucket_container &operator=(bucket_container&& bc) {
    deallocate();
    move_assign(bc, typename traits::propagate_on_container_move_assignment());
    return *this;
  }

  void swap(bucket_container& other) noexcept {
    swap_allocator(value_allocator_, other.value_allocator_,
        typename traits::propagate_on_container_swap());

    data_storage_base_t::swap(other);
  }

  template <typename Field, typename... Args>
  void set_field(Field& f, Args&& ...args) {
    aligned_storage_type<Field> storage;
    traits::construct(
      value_allocator_,
      reinterpret_cast<Field*>(&storage),
      std::forward<Args>(args)...);
    atomic_store_memcpy(std::addressof(f), *reinterpret_cast<Field*>(&storage));
  }
  template <typename Field>
  static void set_field(Field& f, const Field& args) {
    atomic_store_memcpy(std::addressof(f), args);
  }

  static void deoccupy(typed_bucket& b, uint16_t slot) {
    assert(b.occupied(slot));
    atomic_store_memcpy(&b.occupied(slot), false);
  }

  void deoccupy(size_type ind, uint16_t slot) {
    deoccupy((*this)[ind], slot);
  }

  void clear() noexcept {
    for (typed_bucket& b : *this) {
      for (uint16_t slot = 0; slot < typed_bucket::SLOT_PER_BUCKET; ++slot) {
        if (b.occupied(slot)) {
          deoccupy(b, slot);
        }
      }
    }
  }

  void deallocate() noexcept {
    if (this->is_deallocated()) {
      return;
    }
    this->change_size(-1);
  }

  allocator_type& get_allocator() {
    return value_allocator_;
  }

  const allocator_type& get_allocator() const {
    return value_allocator_;
  }

  allocator_type value_allocator_;
};

} // namespace seqlock_lib
