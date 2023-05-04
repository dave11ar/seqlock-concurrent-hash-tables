#pragma once

#include <optional>

#include <libcuckoo/cuckoohash_map.hh>

template<typename Key, typename Value>
class cuckoohash_map {
  libcuckoo::cuckoohash_map<Key, Value> map;

public:
  cuckoohash_map() = default;
  cuckoohash_map(size_t n) : map(n) {}

  void insert(const Key& key, const Value& value) {
    map.insert(key, value);
  }

  std::optional<Value> find(const Key& key) {
    Value result;
    return map.find(key, result) ? result : std::optional<Value>();
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
