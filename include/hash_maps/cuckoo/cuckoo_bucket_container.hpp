#pragma once

#include <bucket_container.hpp>

#include "cuckoo_bucket.hpp"

namespace seqlock_lib::cuckoo {

template <class Key, class T, class Allocator, class Partial, size_t SLOT_PER_BUCKET>
class cuckoo_bucket_container : public bucket_container<
    cuckoo_bucket_container<Key, T, Allocator, Partial, SLOT_PER_BUCKET>, 
    cuckoo_bucket<Key, T, Partial, SLOT_PER_BUCKET>, Allocator> {
public:
  using typed_bucket = cuckoo_bucket<Key, T, Partial, SLOT_PER_BUCKET>;

private:
  using base = bucket_container<
    cuckoo_bucket_container<Key, T, Allocator, Partial, SLOT_PER_BUCKET>, 
    typed_bucket, Allocator>;

  friend base;

  using traits = typename base::traits;

public:
  using base::base;

  using size_type = typename base::size_type;
  using partial_t = Partial;

private:
  // Constructs live data in a bucket
  template <typename K, typename... Args>
  static void setKV_by_this(base* this_base, typed_bucket& b, size_type slot, 
                            partial_t p, K &&k, Args&& ...args) {
    assert(!b.occupied(slot));
    b.partial(slot) = p;
    traits::construct(
      this_base->get_allocator()/*value_allocator_*/, std::addressof(b.storage_kvpair(slot)),
      std::piecewise_construct,
      std::forward_as_tuple(std::forward<K>(k)),
      std::forward_as_tuple(std::forward<Args>(args)...));
    // This must occur last, to enforce a strong exception guarantee
    b.occupied(slot) = true;
  }

  static void copy_bucket_slot(base* this_base, const typed_bucket& src, typed_bucket& dst, size_type slot) {
    setKV_by_this(this_base, dst, slot, src.partial(slot), src.key(slot), src.mapped(slot));
  }

  static void move_bucket_slot(base* this_base, typed_bucket&& src, typed_bucket& dst, size_type slot) {
    setKV_by_this(this_base, dst, slot, src.partial(slot), src.movable_key(slot), std::move(src.mapped(slot)));
  }

public:
  template <typename K, typename... Args>
  void setKV(typed_bucket& b, size_type slot, partial_t p, K &&k,
             Args&& ...args) {
    setKV_by_this(static_cast<base*>(this), b, slot, p, std::forward<K>(k), std::forward<Args>(args)...);
  }

  template <typename K, typename... Args>
  void setKV(size_type ind, size_type slot, partial_t p, K&& k,
             Args&& ...args) {
    setKV((*this)[ind], slot, p, std::forward<K>(k), std::forward<Args>(args)...);
  }
};

} // seqlock_lib::cuckoo
