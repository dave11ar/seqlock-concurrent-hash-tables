#pragma once

#include "../../../src/cuckoo/cuckoohash_map.hpp"

template<typename Key, typename Value>
class concurrent_cuckoo_seqlock_hash_map {
    cuckoo_seqlock::cuckoohash_map<Key, Value> map;

public:
    decltype(auto) insert(const Key& key, const Value& value) {
        return map.insert(key, value);
    }

    decltype(auto) find(const Key& key) {
        return map.contains(key);
    }

    decltype(auto) erase(const Key& key) {
        return map.erase(key);
    }
};
