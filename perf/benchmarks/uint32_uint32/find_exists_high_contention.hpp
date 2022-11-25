#include "../../benchmark_hash_map_framework.hpp"

static void find_exists_high_contentions_uint32_uint32_arguments_threads(benchmark::internal::Benchmark* b) {
    for (int64_t threads = 1; threads <= 16; ++threads) {
        b->Args(get_uint32_benchmark_args_named(threads, 15, 3000000, 15,
            1, 0, 0, 1, 0, 0));
    }
}

static void find_exists_high_contentions_uint32_uint32_arguments_scale(benchmark::internal::Benchmark* b) {
    for (int64_t scale = 300000; scale <= 3000000; scale += 300000) {
        b->Args(get_uint32_benchmark_args_named(16, 15, scale, 15,
            1, 0, 0, 1, 0, 0));
    }
}

START_BENCHMARK(std::string("find_exists_high_contentions_uint32_uint32-threads"), find_exists_high_contentions_uint32_uint32_arguments_threads)
START_BENCHMARK(std::string("find_exists_high_contentions_uint32_uint32-scale"), find_exists_high_contentions_uint32_uint32_arguments_scale)
