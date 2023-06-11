#pragma once

#include "../../benchmark_hash_map.hpp"

static int32_t threads_step = 1;
static int32_t threads_count = 48;

inline void fill_args(benchmark::internal::Benchmark* b, Uint32Args args) {
  if (threads_step > 1) {
    args.threads_count = 1;
    b->Args(get_uint32_benchmark_args(args));
  }
  for (int32_t threads = threads_step; threads <= threads_count; threads += threads_step) {
    args.threads_count = threads;
    b->Args(get_uint32_benchmark_args(args));
  }
}