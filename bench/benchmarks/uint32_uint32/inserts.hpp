#pragma once

#include "utils.hpp"

#ifdef UIN32_UINT32_BENCHMARK_INSERTS
static void inserts_uint32_uint32_arguments(benchmark::internal::Benchmark* b) {
  fill_args(b, {
    .init_scenario_size = 0,
    .running_scenario_scale = 200000,
    .key_max_value = 100000,
    .running_find = 1,
    .running_insert = 10,
    .running_erase = 0,
    .running_update = 0,
    .running_upsert = 0,
    .init_key_generator = 0,
    .running_key_generator = 0,
    .scenarious_generator = 0
  });
}
#endif
