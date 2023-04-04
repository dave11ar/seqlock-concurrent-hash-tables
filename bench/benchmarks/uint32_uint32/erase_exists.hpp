#pragma once

#include "utils.hpp"

#ifdef UIN32_UINT32_BENCHMARK_ERASE_EXISTS
static void erase_exists_uint32_uint32_arguments(benchmark::internal::Benchmark* b) {
  fill_args(b, {
    .init_scenario_size = 2000000,
    .running_scenario_scale = 2000000,
    .key_max_value = 2000000,
    .running_find = 0,
    .running_insert = 0,
    .running_erase = 1,
    .running_update = 0,
    .running_upsert = 0,
    .init_key_generator = 1,
    .running_key_generator = 1,
    .scenarious_generator = 1
  });
}
#endif
