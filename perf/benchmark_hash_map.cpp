#include <algorithm>
#include <cassert>
#include <iostream>
#include <random>
#include <string>
#include <variant>
#include <tbb/tbb.h>
#include "benchmark_custom_candidates/stl/concurrent_stl_map_wrapper.hpp"
#include "benchmark_custom_candidates/cuckoo/concurrent_hash_map.hpp"

class Timer {
    tbb::tick_count tick;
public:
    Timer() : tick(tbb::tick_count::now()){}

    double get_time() {
        return (tbb::tick_count::now() - tick).seconds(); 
    }

    double diff_time(const Timer &newer) {
        return (newer.tick - tick).seconds();
    }

    double mark_time() {
        tbb::tick_count t1(tbb::tick_count::now()), t2(tick); 
        tick = t1; 
        return (t1 - t2).seconds(); 
    }

    double mark_time(const Timer &newer) {
        tbb::tick_count t(tick); 
        tick = newer.tick; 
        return (tick - t).seconds();
    }
};


namespace OperationsInfo {
    enum class Types {
        INSERT,
        FIND
    };

    template<typename Key, typename Value>
    struct Insert {
        Key key;
        Value value;
        Insert(const Key& key, const Value& value) 
            : key(key), value(value) {};
    };

    template<typename Key>
    struct Find {
        Key key;
        Find(const Key& key) 
            : key(key) {};
    };
}


template<typename Key, typename Value>
using Operation = std::variant<
    typename OperationsInfo::Insert<Key, Value>,
    typename OperationsInfo::Find<Key>>;

template<typename Key, typename Value>
using Scenario = std::vector<Operation<Key, Value>>;

template<typename Key, typename Value>
class OperationGenerator {
    std::function<Key()> key_gen;
    std::function<Value()> value_gen;
public:
    OperationGenerator(const std::function<Key()>& key_gen, const std::function<Value()>& value_gen)
        : key_gen(key_gen), value_gen(value_gen) {}
    
    Operation<Key, Value> operator()(const typename OperationsInfo::Types& type) const {
        switch (type) {
            case OperationsInfo::Types::INSERT:
                return Operation<Key, Value>(
                    std::in_place_type<OperationsInfo::Insert<Key, Value>>,
                    key_gen(), value_gen());
            case OperationsInfo::Types::FIND:
                return Operation<Key, Value>(
                    std::in_place_type<OperationsInfo::Find<Key>>,
                    key_gen());
            default:
                assert(false);
        }
    }
};

template<typename Key, typename Value, template<typename...> class Map>
void do_operation(Map<Key, Value>& map, const Operation<Key, Value>& operation) {
    switch (operation.index()) {
        case 0: {
            const auto& insertInfo = std::get<OperationsInfo::Insert<Key, Value>>(operation);
            map.insert(std::make_pair(insertInfo.key, insertInfo.value));
            break;
        }
        case 1: {
            const auto& findInfo = std::get<OperationsInfo::Find<Key>>(operation);
            map.find(findInfo.key);
            break;
        }
        default:
            assert(false);
    }
}

size_t calc_distr_count(const std::unordered_map<OperationsInfo::Types, size_t>& operations_distr) {
    size_t result;
    for (const auto& [operation_type, count] : operations_distr) {
        result += count;
    }

    return result;
}

template<typename Key, typename Value>
Scenario<Key, Value> scenario_generator(
    const std::unordered_map<OperationsInfo::Types, size_t>& operations_distr,
    const size_t& scale,
    const OperationGenerator<Key, Value>& operation_generator) {
    Scenario<Key, Value> result;
    //result.reserve(calc_distr_count(operations_distr));

    for (const auto& [operation_type, count] : operations_distr) {
        for (size_t i = 0; i < count * scale; ++i) {
            result.push_back(operation_generator(operation_type));
        }
    }
    
    return result;
}

template<typename Key, typename Value, template<typename...> class Map>
double test(const std::vector<Scenario<Key, Value>>& scenarious) {
    Map<Key, Value> map;

    Timer timer;
    tbb::parallel_for(tbb::blocked_range<size_t>(0, scenarious.size(), 1), 
        [&map, &scenarious](const auto& range) {
            for (auto it = range.begin(); it != range.end(); ++it) {
                for (const auto& operation : scenarious[it]) {
                    do_operation(map, operation);
                }
            }
        });

    return timer.get_time();
}

template<typename Key, typename Value>
std::vector<Scenario<Key, Value>> get_shuffled_scenarious(
    size_t threads_count, 
    const Scenario<Key, Value>& scenario) {
    std::vector<Scenario<Key, Value>> scenarious(threads_count);
    for (auto& cur_scenario: scenarious) {
        cur_scenario = scenario;
        std::random_shuffle(cur_scenario.begin(), cur_scenario.end());
    }
    
    return scenarious;
}

void benchmark_0() {
    std::mt19937 gen_32;
    auto scenarious = get_shuffled_scenarious<int32_t, int32_t>(
        16,
        scenario_generator(
            {{OperationsInfo::Types::INSERT, 1},
          {OperationsInfo::Types::FIND,   5}},
        10000,
        OperationGenerator<int32_t, int32_t>(
            [&gen_32]{return gen_32(); }, 
            [&gen_32]{return gen_32(); })));

    printf("benchmark_0:\n");
    printf("STL map: %fs\n", test<int32_t, int32_t, CuncurrentSTLMapWrapper>(scenarious));
    printf("TBB map: %fs\n", test<int32_t, int32_t, tbb::concurrent_unordered_map>(scenarious));

    // Gets into deadlock during benchmarking
    // printf("Cuckoo map: %f\n", test<int32_t, int32_t, std::concurrent_unordered_map>(scenarious));
}

void benchmark_1() {
    std::mt19937 gen_32;
    auto scenarious = get_shuffled_scenarious<int32_t, int32_t>(
        16,
        scenario_generator(
            {{OperationsInfo::Types::INSERT, 6},
          {OperationsInfo::Types::FIND,   0}},
        10000,
        OperationGenerator<int32_t, int32_t>(
            [&gen_32]{return gen_32(); }, 
            [&gen_32]{return gen_32(); })));


    printf("benchmark_1:\n");
    printf("STL map: %fs\n", test<int32_t, int32_t, CuncurrentSTLMapWrapper>(scenarious));
    printf("TBB map: %fs\n", test<int32_t, int32_t, tbb::concurrent_unordered_map>(scenarious));

    // Gets into deadlock during benchmarking
    // printf("Cuckoo map: %f\n", test<int32_t, int32_t, std::concurrent_unordered_map>(scenarious));
}

int main() {
    benchmark_0();
    benchmark_1();
    return 0;
}
