// #pragma once

// #include <optional>

// #include <tbb/tbb.h>

// template<typename Key, typename Value>
// class tbb_hash_map {
//     tbb::concurrent_hash_map<Key, Value> map;

// public:
//     void insert(const Key& key, const Value& value) {
//         typename tbb::concurrent_hash_map<Key, Value>::const_accessor acessor;
//         map.insert(acessor, std::make_pair(key, value));
//     }

//     std::optional<Value> find(const Key& key) {
//         typename tbb::concurrent_hash_map<Key, Value>::const_accessor acessor;
//         map.find(acessor, key);
//         return acessor.empty() ? std::optional<Value>() : acessor->second;
//     }

//     void erase(const Key& key) {
//         map.erase(key);
//     }
// };
