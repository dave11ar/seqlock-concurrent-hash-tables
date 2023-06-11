#pragma once

#include "bucket.hpp"

namespace seqlock_lib::cuckoo {

template <typename Key, typename T, typename Partial, uint32_t _SLOT_PER_BUCKET>
class cuckoo_bucket : public bucket<Key, T, _SLOT_PER_BUCKET> {
  using base = bucket<Key, T, _SLOT_PER_BUCKET>;

public:
  static constexpr uint32_t SLOT_PER_BUCKET = _SLOT_PER_BUCKET;

  using base::base;

  using partial_t = Partial;

  const partial_t& partial(uint32_t slot) const {
    return partials_[slot];
  }
  partial_t& partial(uint32_t slot) {
    return partials_[slot];
  }

private:
  std::array<partial_t, SLOT_PER_BUCKET> partials_;
};

} // namespace seqlock_lib::cuckoo
