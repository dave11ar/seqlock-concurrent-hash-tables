#pragma once

#include <unordered_map>
#include <mutex>
#include <tbb/tbb.h>

template<typename Key, typename Value>
class concurrent_tbb_hash_map {
    tbb::concurrent_hash_map<Key, Value> map;

public:
    decltype(auto) insert(const Key& key, const Value& value) {
        typename tbb::concurrent_hash_map<Key, Value>::const_accessor a;
        return map.insert(a, std::make_pair(key, value));
    }

    decltype(auto) find(const Key& key) {
        return map.count(key);
    }

    decltype(auto) erase(const Key& key) {
        return map.erase(key);
    }
};
