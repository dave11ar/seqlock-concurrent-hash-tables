#include "../../benchmark_hash_map_framework.hpp"
#include <string>

static void data_analytics_uint32_uint32_arguments_threads(benchmark::internal::Benchmark* b) {
    for (int64_t threads = 1; threads <= 16; ++threads) {
        b->Args(get_uint32_benchmark_args_named(threads, 0, 500000, 10000, 
            1, 10, 0, 0, 0, 0));
    }
}

static void data_analytics_uint32_uint32_arguments_scale(benchmark::internal::Benchmark* b) {
    for (int64_t scale = 50000; scale <= 500000; scale += 50000) {
        b->Args(get_uint32_benchmark_args_named(16, 0, scale, 10000, 
            1, 10, 0, 0, 0, 0));
    }
}

START_BENCHMARK(std::string("data_analytics_uint32_uint32-threads"), data_analytics_uint32_uint32_arguments_threads)
START_BENCHMARK(std::string("data_analytics_uint32_uint32-scale"), data_analytics_uint32_uint32_arguments_scale)