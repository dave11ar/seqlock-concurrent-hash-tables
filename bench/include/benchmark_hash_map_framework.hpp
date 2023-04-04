#pragma once

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <random>
#include <string>
#include <variant>
#include <benchmark/benchmark.h>

#include "../candidates_wrappers/cuckoo_seqlock_hash_map.hpp"
#include "../candidates_wrappers/cuckoo_hash_map.hpp"
#include "../candidates_wrappers/tbb_hash_map.hpp"

namespace OperationsInfo {
  enum class Types {
    FIND,
    INSERT,
    ERASE,
    UPDATE,
    UPSERT
  };

  template<typename Key>
  struct Find {
    Key key;
    Find(const Key& key) 
        : key(key) {};
  };

  template<typename Key, typename Value>
  struct Insert {
    Key key;
    Value value;
    Insert(const Key& key, const Value& value) 
        : key(key), value(value) {};
  };

  template<typename Key>
  struct Erase {
    Key key;
    Erase(const Key& key) 
        : key(key) {};
  };

  template<typename Key, typename Value>
  struct Update {
    Key key;
    Value value;
    Update(const Key& key, const Value& value) 
        : key(key), value(value) {};
  };

  template<typename Key, typename Value>
  struct Upsert {
    Key key;
    Value value;
    Upsert(const Key& key, const Value& value) 
        : key(key), value(value) {};
  };
}

using RawScenario = std::vector<OperationsInfo::Types>;

template<typename Key, typename Value>
using Operation = std::variant<
    typename OperationsInfo::Find<Key>,
    typename OperationsInfo::Insert<Key, Value>,
    typename OperationsInfo::Erase<Key>,
    typename OperationsInfo::Update<Key, Value>,
    typename OperationsInfo::Upsert<Key, Value>>;

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
      case OperationsInfo::Types::FIND:
        return Operation<Key, Value>(
            std::in_place_type<OperationsInfo::Find<Key>>,
            key_gen());
      case OperationsInfo::Types::INSERT:
        return Operation<Key, Value>(
            std::in_place_type<OperationsInfo::Insert<Key, Value>>,
            key_gen(), value_gen());
      case OperationsInfo::Types::ERASE:
        return Operation<Key, Value>(
            std::in_place_type<OperationsInfo::Erase<Key>>,
            key_gen());
      case OperationsInfo::Types::UPDATE:
        return Operation<Key, Value>(
            std::in_place_type<OperationsInfo::Update<Key, Value>>,
            key_gen(), value_gen());
      case OperationsInfo::Types::UPSERT:
        return Operation<Key, Value>(
            std::in_place_type<OperationsInfo::Upsert<Key, Value>>,
            key_gen(), value_gen());
      default:
        assert(false);
    }
  }
};

template<typename Key, typename Value, template<typename...> typename Map>
void do_operation(Map<Key, Value>& map, const Operation<Key, Value>& operation) {
  switch (operation.index()) {
    case 0: {
      const auto& find_info = std::get<OperationsInfo::Find<Key>>(operation);
      map.find(find_info.key);
      break;
    }
    case 1: {
      const auto& insert_info = std::get<OperationsInfo::Insert<Key, Value>>(operation);
      map.insert(insert_info.key, insert_info.value);
      break;
    }
    case 2: {
      const auto& erase_info = std::get<OperationsInfo::Erase<Key>>(operation);
      map.erase(erase_info.key);
      break;
    }
    case 3: {
      const auto& update_info = std::get<OperationsInfo::Update<Key, Value>>(operation);
      map.update(update_info.key, update_info.value);
      break;
    }
    case 4: {
      const auto& upsert_info = std::get<OperationsInfo::Upsert<Key, Value>>(operation);
      map.upsert(upsert_info.key, upsert_info.value);
      break;
    }
    default:
      assert(false);
  }
}

inline RawScenario generate_raw_scenario(
    const std::unordered_map<OperationsInfo::Types, size_t>& operations_distr,
    const size_t& scale) {
  RawScenario result;
  for (const auto& [operation_type, count] : operations_distr) {
    for (size_t i = 0; i < count * scale; ++i) {
      result.push_back(operation_type);
    }
  }
  
  return result;
}

inline std::vector<RawScenario> get_raw_scenarious_from_raw_scenario(
    size_t threads_count, 
    const RawScenario& scenario) {
  std::vector<RawScenario> scenarious(threads_count);
  std::mt19937 gen_32(222);
  for (auto& cur_scenario: scenarious) {
    cur_scenario = scenario;
    std::shuffle(cur_scenario.begin(), cur_scenario.end(), gen_32);
  }
  
  return scenarious;
}

inline std::vector<RawScenario> split_raw_scenario_on_raw_scenarious(
    size_t threads_count, 
    const RawScenario& scenario) {
  size_t scenario_size = scenario.size() / threads_count;
  std::vector<RawScenario> scenarious(threads_count);
  std::mt19937 gen_32(111);

  auto it = scenario.begin();
  for (auto& cur_scenario: scenarious) {
    cur_scenario = RawScenario(it, it + scenario_size);
    std::shuffle(cur_scenario.begin(), cur_scenario.end(), gen_32);
    it += scenario_size;
  }
  
  return scenarious;
}

template<typename Key, typename Value>
Scenario<Key, Value> get_scenario_from_raw(
    const RawScenario& scenario,
    const OperationGenerator<Key, Value>& operation_generator) {
  Scenario<Key, Value> result;
  result.reserve(scenario.size());
  for (const auto& operation_type : scenario) {
    result.push_back(operation_generator(operation_type));
  }
  
  return result;
}

template<typename Key, typename Value>
std::vector<Scenario<Key, Value>> get_scenarious_from_raw(
    const std::vector<RawScenario>& scenarious,
    const OperationGenerator<Key, Value>& operation_generator) {
  std::vector<Scenario<Key, Value>> result;
  result.reserve(scenarious.size());
  for (const auto& scenario : scenarious) {
    result.push_back(get_scenario_from_raw(scenario, operation_generator));
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
  std::vector<std::thread> vec;
  vec.reserve(scenarious.size() - 1);

  for (size_t i = 0; i < scenarious.size() - 1; ++i) {
    vec.emplace_back(
      [i, &map, &scenarious] {
        execute_scenario(map, scenarious[i]);
      }
    );
  }

  execute_scenario(map, scenarious.back());

  for (auto& t : vec) {
    t.join();
  }
}

template<typename Key, typename Value>
inline std::function<std::vector<RawScenario>(
    size_t threads_count, 
    const RawScenario&)> get_scenarious_function(int64_t arg) {
  switch (arg) {
    case 0:
      return get_raw_scenarious_from_raw_scenario;
    case 1:
      return split_raw_scenario_on_raw_scenarious;
  }
  assert(false);
  return {};
}

inline std::function<uint32_t()> get_uint32_key_function(int64_t arg, int64_t max_value) {
  switch (arg) {
    case 0:
      return [max_value]{static std::mt19937 gen_32(1337); return gen_32() % max_value;};
    case 1:
      return [] {static uint32_t index = 0; return index++;};
  }
  assert(false);
  return {};
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
  const int64_t running_update = state.range(7);
  const int64_t running_upsert = state.range(8);

  for (auto _ : state) {
    state.PauseTiming();
    const auto init_key_generator = get_uint32_key_function(state.range(9), key_max_value);
    const auto running_key_generator = get_uint32_key_function(state.range(10), key_max_value);

    const auto scenarious_generator = get_scenarious_function<uint32_t, uint32_t>(state.range(11));

    const auto scenarious = get_scenarious_from_raw(scenarious_generator(
        threads_count,
        generate_raw_scenario({
            {OperationsInfo::Types::FIND, running_find},
            {OperationsInfo::Types::INSERT, running_insert},
            {OperationsInfo::Types::ERASE, running_erase},
            {OperationsInfo::Types::UPDATE, running_update},
            {OperationsInfo::Types::UPSERT, running_upsert}},
        running_scenario_scale)),
        OperationGenerator<uint32_t, uint32_t>(
            running_key_generator,
            [](){return 0;}));

    char offset[128];
    Map<uint32_t, uint32_t> map;
    execute_scenario(
        map,
        get_scenario_from_raw(generate_raw_scenario(
            {{OperationsInfo::Types::INSERT, 1}},
            init_scenario_size),
            OperationGenerator<uint32_t, uint32_t>(
                init_key_generator,
                [](){return 0;})));
    state.ResumeTiming();

    run_pfor_benchmark(map, scenarious);
  }
}

struct Uint32Args{
  int64_t threads_count;
  int64_t init_scenario_size;
  int64_t running_scenario_scale;
  int64_t key_max_value;
  int64_t running_find;
  int64_t running_insert;
  int64_t running_erase;
  int64_t running_update;
  int64_t running_upsert;
  int64_t init_key_generator;
  int64_t running_key_generator;
  int64_t scenarious_generator;
};

inline std::vector<int64_t> get_uint32_benchmark_args(const Uint32Args& args) {
  return {
    args.threads_count,
    args.init_scenario_size,
    args.running_scenario_scale,
    args.key_max_value,
    args.running_find,
    args.running_insert,
    args.running_erase,
    args.running_update,
    args.running_upsert,
    args.init_key_generator,
    args.running_key_generator,
    args.scenarious_generator
  };
}

#define START_BENCHMARK(name, arguments_generator)\
BENCHMARK_TEMPLATE(abstract_uint32_uint32_benchmark, cuckoo_seqlock_hash_map)\
    ->Name(name + "-cuckoo_seqlock")->Apply(arguments_generator)->Unit(benchmark::kMillisecond)->Iterations(5)->UseRealTime()->MeasureProcessCPUTime();\
BENCHMARK_TEMPLATE(abstract_uint32_uint32_benchmark, cuckoo_hash_map)\
    ->Name(name + "-cuckoo")->Apply(arguments_generator)->Unit(benchmark::kMillisecond)->Iterations(5)->UseRealTime()->MeasureProcessCPUTime();\
// BENCHMARK_TEMPLATE(abstract_uint32_uint32_benchmark, tbb_hash_map)\
//     ->Name(name + "-tbb")->Apply(arguments_generator)->Unit(benchmark::kMillisecond)->UseRealTime();\

