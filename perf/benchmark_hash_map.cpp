#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <random>
#include <string>
#include <variant>
#include <tbb/tbb.h>
#include <benchmark/benchmark.h>
#include <libcuckoo/cuckoohash_map.hh>

#include "benchmark_custom_candidates/stl/concurrent_stl_hash_map.hpp"
#include "benchmark_custom_candidates/cuckoo/concurrent_cuckoo_hash_map.hpp"
#include "benchmark_custom_candidates/tbb_hash/concurrent_tbb_hash_map.hpp"

namespace OperationsInfo {
    enum class Types {
        INSERT,
        FIND,
        ERASE
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

    template<typename Key>
    struct Erase {
        Key key;
        Erase(const Key& key) 
            : key(key) {};
    };
}


template<typename Key, typename Value>
using Operation = std::variant<
    typename OperationsInfo::Insert<Key, Value>,
    typename OperationsInfo::Find<Key>,
    typename OperationsInfo::Erase<Key>>;

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
            case OperationsInfo::Types::ERASE:
                return Operation<Key, Value>(
                    std::in_place_type<OperationsInfo::Erase<Key>>,
                    key_gen());
            default:
                assert(false);
        }
    }
};

template<typename Key, typename Value, template<typename...> typename Map>
void do_operation(Map<Key, Value>& map, const Operation<Key, Value>& operation) {
    switch (operation.index()) {
        case 0: {
            const auto& insertInfo = std::get<OperationsInfo::Insert<Key, Value>>(operation);
            map.insert(insertInfo.key, insertInfo.value);
            break;
        }
        case 1: {
            const auto& findInfo = std::get<OperationsInfo::Find<Key>>(operation);
            map.find(findInfo.key);
            break;
        }
        case 2: {
            const auto& findInfo = std::get<OperationsInfo::Erase<Key>>(operation);
            map.erase(findInfo.key);
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
Scenario<Key, Value> generate_scenario(
    const std::unordered_map<OperationsInfo::Types, size_t>& operations_distr,
    const size_t& scale,
    const OperationGenerator<Key, Value>& operation_generator) {
    Scenario<Key, Value> result;

    for (const auto& [operation_type, count] : operations_distr) {
        for (size_t i = 0; i < count * scale; ++i) {
            result.push_back(operation_generator(operation_type));
        }
    }
    
    return result;
}

template<typename Key, typename Value, template<typename...> typename Map>
void run_pfor_benchmark(
    const std::vector<Scenario<Key, Value>>& scenarious,
    Map<Key, Value>& map) {
    tbb::parallel_for(tbb::blocked_range<size_t>(0, scenarious.size(), 1), 
        [&map, &scenarious](const auto& range) {
            for (auto it = range.begin(); it != range.end(); ++it) {
                for (const auto& operation : scenarious[it]) {
                    do_operation(map, operation);
                }
            }
        });
}

template<typename Key, typename Value>
std::vector<Scenario<Key, Value>> get_scenarious_from_scenario(
    size_t threads_count, 
    const Scenario<Key, Value>& scenario,
    bool is_shuffled = true) {
    std::vector<Scenario<Key, Value>> scenarious(threads_count);
    for (auto& cur_scenario: scenarious) {
        cur_scenario = scenario;
        if (is_shuffled) {
            std::random_shuffle(cur_scenario.begin(), cur_scenario.end());
        }
    }
    
    return scenarious;
}

template<typename Key, typename Value>
std::vector<Scenario<Key, Value>> split_scenario_on_scenarious(
    size_t threads_count, 
    const Scenario<Key, Value>& scenario,
    bool is_shuffled = true) {
    assert(scenario.size() % thread_count == 0);

    std::vector<Scenario<Key, Value>> scenarious(threads_count);

    auto it = scenario.begin();
    for (auto& cur_scenario: scenarious) {
        cur_scenario = std::vector(it, it + threads_count);
        if (is_shuffled) {
            std::random_shuffle(cur_scenario.begin(), cur_scenario.end());
        }
        it += threads_count;
    }
    
    return scenarious;
}


#define GENERATE_BENCHNARK_CODE(benchmark_name, map_name, Key, Value, Map) \
void benchmark_name##_##map_name( \
    benchmark::State& state, \
    const Scenario<Key, Value>& scenario, \
    const std::vector<std::vector<Scenario<Key, Value>>>& scenariousVector) { \
    Map<Key, Value> map; \
    for (const auto& operation : scenario) { \
        do_operation(map, operation); \
    } \
    for (auto _ : state) { \
        for (const auto& scenarious : scenariousVector) { \
            run_pfor_benchmark<Key, Value, Map>(scenarious, map); \
        } \
    } \
}

#define GENERATE_MAP_AND_START(benchmark_name, map_name, Key, Value, Map, scenario, scenariousVector) \
GENERATE_BENCHNARK_CODE(benchmark_name, map_name, Key, Value, Map); \
BENCHMARK_CAPTURE(benchmark_name##_##map_name, b, scenario, scenariousVector); 

#define START_BENCHMARK(benchmark_name, Key, Value, scenario, scenariousVector) \
GENERATE_MAP_AND_START(benchmark_name, stl, Key, Value, concurrent_stl_hash_map, scenario, scenariousVector); \
GENERATE_MAP_AND_START(benchmark_name, cuckoo, Key, Value, concurrent_cuckoo_hash_map, scenario, scenariousVector); \
GENERATE_MAP_AND_START(benchmark_name, tbb, Key, Value, concurrent_tbb_hash_map, scenario, scenariousVector);


START_BENCHMARK(
    uint32_uint32_find_exists, 
    uint32_t, 
    uint32_t, 
    generate_scenario({{OperationsInfo::Types::INSERT, 1}}, 1000000, 
        OperationGenerator<uint32_t, uint32_t>(
            [](){static uint32_t key = 0; return ++key;}, 
            [](){return 0;})), 
    {split_scenario_on_scenarious(
        16,
        generate_scenario({{OperationsInfo::Types::FIND, 1}}, 16000000, 
        OperationGenerator<uint32_t, uint32_t>(
            [](){static std::mt19937 gen_32; return gen_32() % 1000000;}, 
            [](){return 0;})))})

START_BENCHMARK(
    uint32_uint32_find_exists_high_contention, 
    uint32_t, 
    uint32_t, 
    generate_scenario({{OperationsInfo::Types::INSERT, 1}}, 20, 
        OperationGenerator<uint32_t, uint32_t>(
            [](){static uint32_t key = 0; return ++key;}, 
            [](){return 0;})), 
    {split_scenario_on_scenarious(
        16,
        generate_scenario({{OperationsInfo::Types::FIND, 1}}, 16000000, 
        OperationGenerator<uint32_t, uint32_t>(
            [](){static std::mt19937 gen_32; return gen_32() % 20;}, 
            [](){return 0;})))})

START_BENCHMARK(
    uint32_uint32_erase_exists, 
    uint32_t, 
    uint32_t, 
    generate_scenario({{OperationsInfo::Types::INSERT, 1}}, 1000000, 
        OperationGenerator<uint32_t, uint32_t>(
            [](){static uint32_t key = 0; return ++key;}, 
            [](){return 0;})), 
    {split_scenario_on_scenarious(
        16,
        generate_scenario({{OperationsInfo::Types::ERASE, 1}}, 16000000, 
        OperationGenerator<uint32_t, uint32_t>(
            [](){static uint32_t key = 0; return ++key;}, 
            [](){return 0;})))})

  START_BENCHMARK(
    uint32_uint32_insert_then_erase, 
    uint32_t, 
    uint32_t, 
    {}, 
    (std::vector{split_scenario_on_scenarious(
        16,
        generate_scenario({{OperationsInfo::Types::INSERT, 1}}, 8000000, 
        OperationGenerator<uint32_t, uint32_t>(
            [](){static uint32_t key = 0; return ++key;}, 
            [](){return 0;}))),
      split_scenario_on_scenarious(
        16,
        generate_scenario({{OperationsInfo::Types::ERASE, 1}}, 8000000, 
        OperationGenerator<uint32_t, uint32_t>(
            [](){static uint32_t key = 0; return ++key;}, 
            [](){return 0;})))}))

    START_BENCHMARK(
        uint32_uint32_insert_then_find, 
        uint32_t, 
        uint32_t, 
        {}, 
        (std::vector{split_scenario_on_scenarious(
            16,
            generate_scenario({{OperationsInfo::Types::INSERT, 1}}, 8000000, 
            OperationGenerator<uint32_t, uint32_t>(
                [](){static uint32_t key = 0; return ++key;}, 
                [](){return 0;}))),
        split_scenario_on_scenarious(
            16,
            generate_scenario({{OperationsInfo::Types::FIND, 1}}, 8000000, 
            OperationGenerator<uint32_t, uint32_t>(
                [](){static uint32_t key = 0; return ++key;}, 
                [](){return 0;})))}))

    START_BENCHMARK(
        uint32_uint32_insert_then_find_then_erase, 
        uint32_t, 
        uint32_t, 
        {}, 
        (std::vector{split_scenario_on_scenarious(
            16,
            generate_scenario({{OperationsInfo::Types::INSERT, 1}}, 100000, 
            OperationGenerator<uint32_t, uint32_t>(
                [](){static uint32_t key = 0; return ++key;}, 
                [](){return 0;}))),
        split_scenario_on_scenarious(
            16,
            generate_scenario({{OperationsInfo::Types::FIND, 1}}, 10000000, 
            OperationGenerator<uint32_t, uint32_t>(
                [](){static uint32_t key = 0; return ++key;}, 
                [](){return 0;}))),
        split_scenario_on_scenarious(
            16,
            generate_scenario({{OperationsInfo::Types::ERASE, 1}}, 100000, 
            OperationGenerator<uint32_t, uint32_t>(
                [](){static uint32_t key = 0; return ++key;}, 
                [](){return 0;})))}))

BENCHMARK_MAIN();