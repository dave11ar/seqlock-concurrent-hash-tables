#pragma once

#include <cuckoo/cuckoohash_map.hpp>

template<typename Key, typename Value>
class cuckoo_seqlock_hash_map {
  cuckoo_seqlock::cuckoohash_map<Key, Value> map;

public:
  cuckoo_seqlock_hash_map() = default;
  cuckoo_seqlock_hash_map(size_t n) : map(n) {}

  std::optional<Value> find(const Key& key) {
    Value result;
    return map.find(key, result) ? result : std::optional<Value>();
  }

  void insert(const Key& key, const Value& value) {
    map.insert(key, value);
  }

  void erase(const Key& key) {
    map.erase(key);
  }

  void update(const Key& key, const Value& value) {
    map.update(key, value);
  }

  void upsert(const Key& key, const Value& value) {
    map.upsert(key,[](const Value&) {}, value);
  }
};
