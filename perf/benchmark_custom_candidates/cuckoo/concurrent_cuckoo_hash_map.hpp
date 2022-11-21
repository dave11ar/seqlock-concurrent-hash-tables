#pragma once

#include <libcuckoo/cuckoohash_map.hh>
#include <type_traits>

template<typename Key, typename Value>
class concurrent_cuckoo_hash_map {
    libcuckoo::cuckoohash_map<Key, Value> map;

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
