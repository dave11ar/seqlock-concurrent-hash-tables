#pragma once

#include "bucket.hpp"
#include "seqlock.hpp"

namespace seqlock_lib::rh {

using seqlock_t = seqlock<1>;

template <typename Key, typename T, size_t _SLOT_PER_BUCKET>
class rh_bucket : public seqlock_t, public bucket<Key, T, _SLOT_PER_BUCKET>{
  using base_bucket = bucket<Key, T, _SLOT_PER_BUCKET>;

public:
  static constexpr size_t SLOT_PER_BUCKET = _SLOT_PER_BUCKET;

  rh_bucket() = default;
  rh_bucket(bool locked) : seqlock(locked), base_bucket() {}

  const uint16_t& dist(size_t ind) const {
    return distances[ind];
  }
  uint16_t& dist(size_t ind) {
    return distances[ind];
  }
private:
  std::array<uint16_t, SLOT_PER_BUCKET> distances;
};

} // namespace seqlock_lib::rh
