#pragma once

#include "utils.hpp"

#ifdef UIN32_UINT32_BENCHMARK_INSERTS
static void inserts_rehashing_uint32_uint32_arguments(benchmark::internal::Benchmark* b) {
  fill_args(b, {
    .init_map_size = 0,
    .init_scenario_size = 0,
    .running_scenario_scale = 50000000,
    .key_max_value = 50000000,
    .running_find = 0,
    .running_insert = 1,
    .running_insert_or_assign = 0,
    .running_erase = 0,
    .init_key_generator = 0,
    .running_key_generator = 1,
    .scenarious_generator = 1
  });
}

static void inserts_no_rehashing_uint32_uint32_arguments(benchmark::internal::Benchmark* b) {
  fill_args(b, {
    .init_map_size = 50000000,
    .init_scenario_size = 0,
    .running_scenario_scale = 5000000,
    .key_max_value = 50000000,
    .running_find = 4,
    .running_insert = 1,
    .running_insert_or_assign = 0,
    .running_erase = 0,
    .init_key_generator = 0,
    .running_key_generator = 1,
    .scenarious_generator = 1
  });
}
#endif
