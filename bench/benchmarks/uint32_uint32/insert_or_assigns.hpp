#pragma once

#include "utils.hpp"

#ifdef UIN32_UINT32_BENCHMARK_INSERT_OR_ASSIGNS
static void insert_or_assigns_uint32_uint32_arguments(benchmark::internal::Benchmark* b) {
  fill_args(b, {
    .init_map_size = 0,
    .init_scenario_size = 0,
    .running_scenario_scale = 10000000,
    .key_max_value = 1000000,
    .running_find = 0,
    .running_insert = 0,
    .running_insert_or_assign = 1,
    .running_erase = 0,
    .init_key_generator = 0,
    .running_key_generator = 0,
    .scenarious_generator = 0
  });
}

static void insert_or_assigns_high_contention_uint32_uint32_arguments(benchmark::internal::Benchmark* b) {
  fill_args(b, {
    .init_map_size = 0,
    .init_scenario_size = 0,
    .running_scenario_scale = 10000000,
    .key_max_value = 1000,
    .running_find = 0,
    .running_insert = 0,
    .running_insert_or_assign = 1,
    .running_erase = 0,
    .init_key_generator = 0,
    .running_key_generator = 0,
    .scenarious_generator = 0
  });
}
#endif
