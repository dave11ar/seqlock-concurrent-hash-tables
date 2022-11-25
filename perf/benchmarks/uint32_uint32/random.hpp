#include "../../benchmark_hash_map_framework.hpp"

static void random_uint32_uint32_arguments_threads(benchmark::internal::Benchmark* b) {
    for (int64_t threads = 1; threads <= 16; ++threads) {
        b->Args(get_uint32_benchmark_args_named(threads, 0, 1000000, 10000,
            1, 1, 1, 0, 0, 0));
    }
}

static void random_uint32_uint32_arguments_scale(benchmark::internal::Benchmark* b) {
    for (int64_t scale = 100000; scale <= 1000000; scale += 100000) {
        b->Args(get_uint32_benchmark_args_named(16, 0, scale, 10000,
            1, 1, 1, 0, 0, 0));
    }
}

START_BENCHMARK(std::string("random_uint32_uint32-threads"), random_uint32_uint32_arguments_threads)
START_BENCHMARK(std::string("random_uint32_uint32-scale"), random_uint32_uint32_arguments_scale)