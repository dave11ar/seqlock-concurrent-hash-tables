#pragma once

#include "utils.hpp"

#ifdef UIN32_UINT32_BENCHMARK_RANDOM
static void random_uint32_uint32_arguments(benchmark::internal::Benchmark* b) {
  fill_args(b, {
    .init_scenario_size = 0,
    .running_scenario_scale = 500000,
    .key_max_value = 100000,
    .running_find = 1,
    .running_insert = 1,
    .running_erase = 1,
    .running_update = 1,
    .running_upsert = 1,
    .init_key_generator = 0,
    .running_key_generator = 0,
    .scenarious_generator = 0
  });
}
#endif
