#pragma once

#include <bucket.hpp>

namespace seqlock_lib::cuckoo {

template <typename Key, typename T, typename Partial, size_t _SLOT_PER_BUCKET>
class cuckoo_bucket : public bucket<Key, T, _SLOT_PER_BUCKET> {
  using base = bucket<Key, T, _SLOT_PER_BUCKET>;

public:
  static constexpr size_t SLOT_PER_BUCKET = _SLOT_PER_BUCKET;

  using base::base;

  using partial_t = Partial;

  partial_t partial(size_t ind) const {
    return partials_[ind];
  }
  partial_t &partial(size_t ind) {
    return partials_[ind];
  }

private:
  std::array<partial_t, SLOT_PER_BUCKET> partials_;
};

} // namespace seqlock_lib::cuckoo
