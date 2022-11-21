#pragma once

#include "oneapi/tbb/spin_mutex.h"
#include <cstdint>
#include <unordered_map>
#include <mutex>
#include <vector>
#include <utility>

// SHARDS NOW VALID ONLY FOR UINT32_T

constexpr uint32_t shards_pow = 15;

template<typename Key, typename Value>
class concurrent_stl_hash_map {
    std::array<std::pair<std::unordered_map<Key, Value>, tbb::spin_mutex>, 1 << shards_pow> shards;
public:
    decltype(auto) insert(const Key& key, const Value& value) {
        auto& [map, mutex] = shards[key >> (32 - shards_pow)];
        std::lock_guard<tbb::spin_mutex> lock(mutex);
        return map.insert(std::make_pair(key, value));
    }

    decltype(auto) find(const Key& key) {
        auto& [map, mutex] = shards[key >> (32 - shards_pow)];
        std::lock_guard<tbb::spin_mutex> lock(mutex);
        return map.count(key);
    }

    decltype(auto) erase(const Key& key) {
        auto& [map, mutex] = shards[key >> (32 - shards_pow)];
        std::lock_guard<tbb::spin_mutex> lock(mutex);
        return map.erase(key);
    }
};
