#pragma once

#include <memory>

#include "utils.hpp"

namespace seqlock_lib {

/*
* The bucket type holds SLOT_PER_BUCKET key-value pairs, along with their
* occupancy info. It uses aligned_storage arrays to store
* the keys and values to allow constructing and destroying key-value pairs
* in place. The lifetime of bucket data should be managed by the container.
* It is the user's responsibility to confirm whether the data they are
* accessing is live or not.
*/
template <typename Key, typename T, uint32_t SLOT_PER_BUCKET>
class bucket {
public:
  using key_type = Key;
  using mapped_type = T;
  using value_type = std::pair<const Key, T>;
  using storage_value_type = std::pair<Key, T>;

  using key_raw_type = aligned_storage_type<key_type>;
  using mapped_raw_type = aligned_storage_type<mapped_type>;
  using storage_value_raw_type = aligned_storage_type<storage_value_type>;

  bucket() noexcept : occupied_() {}

  const storage_value_type& storage_kvpair(uint32_t slot) const {
    return *reinterpret_cast<const storage_value_type*>(&values_[slot]);
  }
  storage_value_type& storage_kvpair(uint32_t slot) {
    return  *reinterpret_cast<storage_value_type*>(&values_[slot]);
  }

  const value_type& kvpair(uint32_t slot) const {
    return  *reinterpret_cast<const value_type*>(&values_[slot]);
  }
  value_type& kvpair(uint32_t slot) {
    return *reinterpret_cast<value_type*>(&values_[slot]);
  }

  const key_type& key(uint32_t slot) const {
    return storage_kvpair(slot).first;
  }
  key_type& key(uint32_t slot) {
    return storage_kvpair(slot).first;
  }

  const mapped_type& mapped(uint32_t slot) const {
    return storage_kvpair(slot).second;
  }
  mapped_type& mapped(uint32_t slot) { 
    return storage_kvpair(slot).second;
  }

  const bool& occupied(uint32_t slot) const { return occupied_[slot]; }
  bool& occupied(uint32_t slot) { return occupied_[slot]; }

private:
  std::array<storage_value_raw_type, SLOT_PER_BUCKET> values_;
  std::array<bool, SLOT_PER_BUCKET> occupied_;
};

} // namespace seqlock_lib
