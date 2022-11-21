#pragma once

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
void execute_scenario(
    Map<Key, Value>& map,
    const Scenario<Key, Value>& scenario) {
    for (const auto& operation : scenario) {
        do_operation(map, operation);
    }
}

template<typename Key, typename Value, template<typename...> typename Map>
void run_pfor_benchmark(
    Map<Key, Value>& map,
    const std::vector<Scenario<Key, Value>>& scenarious) {
    tbb::parallel_for(tbb::blocked_range<size_t>(0, scenarious.size(), 1), 
        [&map, &scenarious](const auto& range) {
            for (auto it = range.begin(); it != range.end(); ++it) {
                execute_scenario(map, scenarious[it]);
            }
        });
}

template<typename Key, typename Value>
std::vector<Scenario<Key, Value>> get_scenarious_from_scenario(
    size_t threads_count, 
    const Scenario<Key, Value>& scenario) {
    std::vector<Scenario<Key, Value>> scenarious(threads_count);
    for (auto& cur_scenario: scenarious) {
        cur_scenario = scenario;
        std::random_shuffle(cur_scenario.begin(), cur_scenario.end());
    }
    
    return scenarious;
}

template<typename Key, typename Value>
std::vector<Scenario<Key, Value>> split_scenario_on_scenarious(
    size_t threads_count, 
    const Scenario<Key, Value>& scenario) {
    size_t scenarioSize = scenario.size() / threads_count;
    std::vector<Scenario<Key, Value>> scenarious(threads_count);

    auto it = scenario.begin();
    for (auto& cur_scenario: scenarious) {
        cur_scenario = Scenario<Key, Value>(it, it + scenarioSize);
        std::random_shuffle(cur_scenario.begin(), cur_scenario.end());
        it += scenarioSize;
    }
    
    return scenarious;
}

template<typename Key, typename Value>
inline std::function<std::vector<Scenario<Key, Value>>(
    size_t threads_count, 
    const Scenario<Key, Value>&)> get_scenarious_function(int64_t arg) {
    switch (arg) {
        case 0:
            return get_scenarious_from_scenario<Key, Value>;
        case 1:
            return split_scenario_on_scenarious<Key, Value>;
    }
    assert(false);
}

inline std::function<uint32_t()> get_uint32_key_function(int64_t arg, int64_t max_value) {
    switch (arg) {
        case 0:
            return [max_value]{static std::mt19937 gen_32; return gen_32() % max_value;};
        case 1:
            return [] {static uint32_t index = 0; return index++;};
    }
    assert(false);
}

template<template<typename ...> typename Map>
static void abstract_uint32_uint32_benchmark(benchmark::State& state) { 
    const int64_t threads_count = state.range(0);

    const int64_t init_scenario_size = state.range(1);
    const int64_t running_scenario_scale = state.range(2);

    const int64_t key_max_value = state.range(3);

    const int64_t running_find = state.range(4);
    const int64_t running_insert = state.range(5);
    const int64_t running_erase = state.range(6);

    const auto init_key_generator = get_uint32_key_function(state.range(7), key_max_value);
    const auto running_key_generator = get_uint32_key_function(state.range(8), key_max_value);

    const auto scenarious_generator = get_scenarious_function<uint32_t, uint32_t>(state.range(9));

    for (auto _ : state) {
        state.PauseTiming();
        const auto scenarious = scenarious_generator(
            threads_count,
            generate_scenario({{OperationsInfo::Types::FIND, running_find},
                    {OperationsInfo::Types::INSERT, running_insert},
                    {OperationsInfo::Types::ERASE, running_erase}},
            running_scenario_scale, 
            OperationGenerator<uint32_t, uint32_t>(
                running_key_generator, 
                [](){return 0;})));
        
        char offset[128];
        Map<uint32_t, uint32_t> map;
        execute_scenario(
            map,
            generate_scenario(
                {{OperationsInfo::Types::INSERT, 1}}, 
                init_scenario_size, 
                OperationGenerator<uint32_t, uint32_t>(
                    init_key_generator, 
                    [](){return 0;})));
        state.ResumeTiming();

        run_pfor_benchmark(map, scenarious);
    } 
}

inline std::vector<int64_t> get_uint32_benchmark_args_named(
    int64_t threads_count,
    int64_t init_scenario_size,
    int64_t running_scenario_scale,
    int64_t key_max_value,
    int64_t running_find,
    int64_t running_insert,
    int64_t running_erase,
    int64_t init_key_generator,
    int64_t running_key_generator,
    int64_t scenarious_generator
) {
    return {threads_count, init_scenario_size, running_scenario_scale, key_max_value,
        running_find, running_insert, running_erase, init_key_generator, running_key_generator, scenarious_generator};
}

#define START_BENCHMARK(name, arguments_generator)\
BENCHMARK_TEMPLATE(abstract_uint32_uint32_benchmark, concurrent_stl_hash_map)\
    ->Name(name + "-cuckoo")->Apply(arguments_generator)->Unit(benchmark::kMillisecond)->UseRealTime();\
BENCHMARK_TEMPLATE(abstract_uint32_uint32_benchmark, concurrent_tbb_hash_map)\
    ->Name(name + "-tbb")->Apply(arguments_generator)->Unit(benchmark::kMillisecond)->UseRealTime();

//BENCHMARK_TEMPLATE(abstract_uint32_uint32_benchmark, concurrent_stl_hash_map)\
//    ->Name(name)->Apply(arguments_generator)->Unit(benchmark::kMillisecond)->UseRealTime();

