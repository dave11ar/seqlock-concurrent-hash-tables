#include <unordered_map>
#include <mutex>

template<typename Key, typename Value>
class CuncurrentSTLMapWrapper {
    std::unordered_map<Key, Value> map;
    std::mutex mutex;
public:
    void insert(const std::pair<Key, Value>& pair) {
        std::lock_guard<std::mutex> lock(mutex);
        map.insert(pair);
    }

    decltype(auto) find(const Key& key) {
        std::lock_guard<std::mutex> lock(mutex);
        return map.find(key);
    }
};
