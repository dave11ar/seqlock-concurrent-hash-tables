#pragma once

#include "../../benchmark_hash_map_framework.hpp"

static void find_insert_hash_contention_uint32_uint32_arguments_threads(benchmark::internal::Benchmark* b) {
    for (int64_t threads = 1; threads <= 16; ++threads) {
        b->Args(get_uint32_benchmark_args_named(threads, 50, 1000000, 50,
            10, 1, 0, 1, 0, 0));
    }
}

static void find_insert_hash_contention_uint32_uint32_arguments_scale(benchmark::internal::Benchmark* b) {
    for (int64_t scale = 100000; scale <= 1000000; scale += 100000) {
        b->Args(get_uint32_benchmark_args_named(16, 50, scale, 50,
            10, 1, 0, 1, 0, 0));
    }
}

START_BENCHMARK(std::string("find_insert_hash_contention_uint32_uint32-threads"), find_insert_hash_contention_uint32_uint32_arguments_threads)
START_BENCHMARK(std::string("find_insert_hash_contention_uint32_uint32-scale"), find_insert_hash_contention_uint32_uint32_arguments_scale)