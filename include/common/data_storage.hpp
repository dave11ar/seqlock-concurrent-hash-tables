#pragma once

#include <bit>
#include <array>

#include "utils.hpp"

namespace seqlock_lib {

template <typename Value, typename Allocator>
class data_storage {
protected:
  using traits = typename std::allocator_traits<
      Allocator>::template rebind_traits<Value>;
  using allocator_type = typename traits::allocator_type;

  using data_pointer = typename traits::pointer;
  using data_pointers_array = std::array<data_pointer, 64>;

  using size_type = typename traits::size_type;

public:
  struct iterator {
    using array_iterator_t = typename data_pointers_array::iterator;

    iterator(const array_iterator_t& array_iterator, size_type data_index, int32_t array_index)
        : array_iterator(array_iterator), data_index(data_index), array_index(array_index) {}

    const Value& operator*() const {
      return (*array_iterator)[data_index];
    }
    Value& operator*() {
      return (*array_iterator)[data_index];
    }

    const Value* operator->() const {
      return &((*array_iterator)[data_index]);
    }
    Value* operator->() {
      return &((*array_iterator)[data_index]);
    }

    iterator& operator++() {
      if (data_index == size_by_index(array_index) - 1) {
        ++array_iterator;
        ++array_index;
        data_index = 0;
      } else {
        ++data_index;
      }

      return *this;
    }
    iterator operator++(int) {
      iterator result(*this);
      ++(*this);
      return result;
    }

    iterator& operator--() {
      if (data_index == 0) {
        --array_iterator;
        --array_index;
        data_index = size_by_index(array_index) - 1;
      } else {
        --data_index;
      }

      return *this;
    }
    iterator operator--(int) {
      iterator result(*this);
      --(*this);
      return result;
    }

    bool operator==(const iterator& other) const {
      return data_index == other.data_index &&
             array_index == other.array_index;
    }
    bool operator!=(const iterator& other) const {
      return !(*this == other);
    }

  public:
    array_iterator_t array_iterator;
    size_type data_index;
    int32_t array_index;
  };

private:
  template <typename ResultT, typename F>
  static ResultT find_value(size_type i, F func) {
    static constexpr uint32_t type_size_ind = (sizeof(size_type) << 3);

    if (i == 0) {
      return func(0, 0); 
    }

    const size_type left_bit_ind = type_size_ind - std::countl_zero(i);
    return func(left_bit_ind, i ^ (size_type(1) << (left_bit_ind - 1)));
  }

  void hashpower(int32_t hp) noexcept {
    hp_.store(hp, std::memory_order_release);
  }

protected:
  static constexpr size_type size_by_index(int32_t  index) {
    return index == 0 ? 1 : (size_type(1) << (index - 1));
  }

  void move_pointers(data_storage&& other) {
    const int32_t hp = other.hashpower();
    for (int32_t i = 0; i <= hp; ++i) {
      data_[i] = other.data_[i];
      other.data_[i] = nullptr;
    }

    hashpower(other.hp_.exchange(-1, std::memory_order_acq_rel));
  }

  void change_size(int32_t new_hp) {
    const int32_t hp = hashpower();

    if (hp == new_hp) {
      return;
    }

    if (hp < new_hp) {
      for (int32_t i = hp + 1; i <= new_hp; ++i) {
        data_[i] = allocate_and_construct(i);
      }
      hp_.store(new_hp, std::memory_order_release);
    } else {
      hp_.store(new_hp, std::memory_order_release);
      for (int32_t i = hp; i > new_hp; --i) {
        deallocate(data_[i],i);
      }
    }
  }

public:
  data_storage(int32_t hp, const allocator_type& allocator)
      : data_allocator_(allocator), hp_(-1) {
    change_size(hp);
  }

  virtual ~data_storage() {
    change_size(-1);
  }

  virtual void swap(data_storage &other) noexcept {
    swap_allocator(data_allocator_, other.data_allocator_,
                   typename traits::propagate_on_container_swap());
    // Regardless of whether we actually swapped the allocators or not, it will
    // always be okay to do the remainder of the swap. This is because if the
    // allocators were swapped, then the subsequent operations are okay. If the
    // allocators weren't swapped but compare equal, then we're okay. If they
    // weren't swapped and compare unequal, then behavior is undefined, so
    // we're okay.
    std::swap(hp_, other.hp_);
    for (int32_t i = 0; i <= std::max(hashpower(), other.hashpower()); ++i) {
      std::swap(data_[i], other.data_[i]);
    }
  }

  int32_t hashpower() const noexcept {
    return hp_.load(std::memory_order_acquire);
  }

  size_type size() const {
    const int32_t hp = hashpower();
    return hp == -1 ? 0 : (size_type(1) << hp);
  }

  template <typename ...Args>
  data_pointer allocate_and_construct(int32_t index, Args&& ...args) {
    const size_type size = size_by_index(index);
    data_pointer pointer = data_allocator_.allocate(size);

    for (size_type i = 0; i < size; ++i) {
      traits::construct(data_allocator_, &pointer[i], std::forward<Args>(args)...);
    }
    return pointer;
  }

  void deallocate(data_pointer& pointer, size_type index) noexcept {
    data_allocator_.deallocate(pointer, size_by_index(index));
    pointer = nullptr;
  }

  template <typename ...Args>
  void double_size(Args&& ...args) {
    const int32_t new_hp = hashpower() + 1;
    data_[new_hp] = allocate_and_construct(new_hp, std::forward<Args>(args)...);
    hashpower(new_hp);
  }

  void push_back(data_pointer pointer) noexcept {
    const int32_t new_hp = hashpower() + 1;
    data_[new_hp] = pointer;
    hashpower(new_hp);
  }

  Value& operator[](size_type i) {
    return find_value<Value&>(i, 
      [this](size_type array_index, size_type data_index) -> Value& {
        return data_[array_index][data_index];
      }
    );
  }
  const Value& operator[](size_type i) const {
    return find_value<const Value&>(i, 
      [this](size_type array_index, size_type data_index) -> const Value& {
        return data_[array_index][data_index];
      }
    );
  }

  iterator get_iterator(size_type index) {
    return find_value<iterator>(index, 
      [this, index](size_type array_index, size_type data_index) -> iterator {
        return iterator(data_.begin() + array_index, data_index, array_index);
      }
    );
  }

  iterator begin() const {
    return iterator(data_.begin(), 0, 0);
  }

  iterator end() const {
    const int32_t hp = hashpower();
    const int32_t array_index = (hp == -1 ? 0 : hp + 1);
    return iterator(data_.begin() + array_index, 0, array_index);
  }

  bool is_deallocated() const noexcept {
    return data_[0] == nullptr;
  }

protected:
  mutable data_pointers_array data_{nullptr};
  allocator_type data_allocator_;
  CopyableAtomic<int32_t> hp_;
};

} // namespace seqlock_lib
