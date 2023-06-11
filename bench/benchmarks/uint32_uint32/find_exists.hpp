#pragma once

#include "utils.hpp"

#ifdef UIN32_UINT32_BENCHMARK_FIND_EXISTS
static void find_exists_uint32_uint32_arguments(benchmark::internal::Benchmark* b) {
  fill_args(b, {
    .init_map_size = 1000000,
    .init_scenario_size = 1000000,
    .running_scenario_scale = 5000000,
    .key_max_value = 1000000,
    .running_find = 1,
    .running_insert = 0,
    .running_insert_or_assign = 0,
    .running_erase = 0,
    .init_key_generator = 1,
    .running_key_generator = 0,
    .scenarious_generator = 0
  });
}

static void find_exists_high_contentions_uint32_uint32_arguments(benchmark::internal::Benchmark* b) {
  fill_args(b, {
    .init_map_size = 1000,
    .init_scenario_size = 1000,
    .running_scenario_scale = 5000000,
    .key_max_value = 1000,
    .running_find = 1,
    .running_insert = 0,
    .running_insert_or_assign = 0,
    .running_erase = 0,
    .init_key_generator = 1,
    .running_key_generator = 0,
    .scenarious_generator = 0
  });
}
#endif
