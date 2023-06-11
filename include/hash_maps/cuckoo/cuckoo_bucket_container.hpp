#pragma once

#include "atomic_buffer.hpp"
#include "bucket_container.hpp"

#include "cuckoo_bucket.hpp"

namespace seqlock_lib::cuckoo {

template <class Key, class T, class Allocator, class Partial, uint32_t SLOT_PER_BUCKET>
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

public:
  using traits = typename base::traits;

  using base::base;
  using base::operator=;

  using size_type = typename base::size_type;
  using partial_t = Partial;

private:
  static void copy_bucket_slot(base* this_base, uint32_t slot, typed_bucket& dst, const typed_bucket& src) {
    setKV_by_this(this_base, dst, slot, src.partial(slot), src.key(slot), src.mapped(slot));
  }

  // Constructs live data in a bucket
  template <typename K, typename... Args>
  static void setKV_by_this(base* this_base, typed_bucket& b, uint32_t slot, 
                            partial_t p, K &&k, Args&& ...args) {
    assert(!b.occupied(slot));

    atomic_store_memcpy(&b.partial(slot), p);
    this_base->set_field(b.key(slot), std::forward<K>(k));
    this_base->set_field(b.mapped(slot), std::forward<Args>(args)...);
    atomic_store_memcpy(&b.occupied(slot), true);
  }

public:
  template <typename K, typename... Args>
  void setKV(typed_bucket& b, uint32_t slot, partial_t p, 
             K&& k, Args&& ...args) {
    setKV_by_this(static_cast<base*>(this), b, slot, p, std::forward<K>(k), std::forward<Args>(args)...);
  }

  template <typename K, typename... Args>
  void setKV(size_type ind, uint32_t slot, partial_t p, K&& k,
             Args&& ...args) {
    setKV((*this)[ind], slot, p, std::forward<K>(k), std::forward<Args>(args)...);
  }
};

} // seqlock_lib::cuckoo
