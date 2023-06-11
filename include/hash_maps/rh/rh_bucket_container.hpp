#pragma once

#include "atomic_buffer.hpp"
#include "bucket_container.hpp"

#include "rh_bucket.hpp"
#include <string>
#include <type_traits>

namespace seqlock_lib::rh {

template <class Key, class T, class Allocator, size_t SLOT_PER_BUCKET>
class rh_bucket_container : public bucket_container<
    rh_bucket_container<Key, T, Allocator, SLOT_PER_BUCKET>, 
    rh_bucket<Key, T, SLOT_PER_BUCKET>, Allocator> {
public:
  using typed_bucket = rh_bucket<Key, T, SLOT_PER_BUCKET>;

private:
  using base = bucket_container<
    rh_bucket_container<Key, T, Allocator, SLOT_PER_BUCKET>, 
    typed_bucket, Allocator>;

  friend base;

  using traits = typename base::traits;

public:
  using base::base;

  using size_type = typename base::size_type;

private:
  static void copy_bucket_slot(base* this_base, uint16_t slot, typed_bucket& dst, const typed_bucket& src) {
    setKV_by_this(this_base, dst, slot, src.dist(slot), src.key(slot), src.mapped(slot));
  }

  // Constructs live data in a bucket
  template <typename K, typename... Args>
  static void setKV_by_this(base* this_base, typed_bucket& b, uint16_t slot, 
                            uint16_t dist, K &&k, Args&& ...args) {
    atomic_store_memcpy(&b.dist(slot), dist);
    this_base->set_field(b.key(slot), std::forward<K>(k));
    this_base->set_field(b.mapped(slot), std::forward<Args>(args)...);
    atomic_store_memcpy(&b.occupied(slot), true);
  }

public:
  template <typename K, typename... Args>
  void setKV(typed_bucket& b, size_type slot, uint16_t dist,
             K &&k, Args&& ...args) {
    setKV_by_this(static_cast<base*>(this), b, slot, dist, std::forward<K>(k), std::forward<Args>(args)...);
  }

  template <typename K, typename... Args>
  void setKV(size_type ind, size_type slot, uint16_t dist,
             K&& k, Args&& ...args) {
    setKV((*this)[ind], slot, dist, std::forward<K>(k), std::forward<Args>(args)...);
  }
};

} // seqlock_lib::rh
