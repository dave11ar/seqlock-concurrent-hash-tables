#pragma once

#include <memory>

namespace cuckoo_seqlock{

/*
* The bucket type holds SLOT_PER_BUCKET key-value pairs, along with their
* partial keys and occupancy info. It uses aligned_storage arrays to store
* the keys and values to allow constructing and destroying key-value pairs
* in place. The lifetime of bucket data should be managed by the container.
* It is the user's responsibility to confirm whether the data they are
* accessing is live or not.
*/
template <typename Key, typename T, typename Partial, size_t SLOT_PER_BUCKET>
class bucket {
public:
  using key_type = Key;
  using mapped_type = T;
  using value_type = std::pair<const Key, T>;
  using storage_value_type = std::pair<Key, T>;

  using partial_t = Partial;

  bucket() noexcept : occupied_() {}

  const storage_value_type &storage_kvpair(size_t ind) const {
    return *static_cast<const storage_value_type *>(
        static_cast<const void *>(&values_[ind]));
  }
  storage_value_type &storage_kvpair(size_t ind) {
    return *static_cast<storage_value_type *>(
        static_cast<void *>(&values_[ind]));
  }

  const value_type &kvpair(size_t ind) const {
    return *static_cast<const value_type *>(
        static_cast<const void *>(&values_[ind]));
  }
  value_type &kvpair(size_t ind) {
    return *static_cast<value_type *>(static_cast<void *>(&values_[ind]));
  }

  const key_type &key(size_t ind) const {
    return storage_kvpair(ind).first;
  }
  key_type &&movable_key(size_t ind) {
    return std::move(storage_kvpair(ind).first);
  }

  const mapped_type &mapped(size_t ind) const {
    return storage_kvpair(ind).second;
  }
  mapped_type &mapped(size_t ind) { return storage_kvpair(ind).second; }

  partial_t partial(size_t ind) const { return partials_[ind]; }
  partial_t &partial(size_t ind) { return partials_[ind]; }

  bool occupied(size_t ind) const { return occupied_[ind]; }
  bool &occupied(size_t ind) { return occupied_[ind]; }

private:
  std::array<typename std::aligned_storage<sizeof(storage_value_type),
                                            alignof(storage_value_type)>::type,
              SLOT_PER_BUCKET>
      values_;
  std::array<partial_t, SLOT_PER_BUCKET> partials_;
  std::array<bool, SLOT_PER_BUCKET> occupied_;
};
}