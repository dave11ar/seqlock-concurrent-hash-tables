#include "../../benchmark_hash_map_framework.hpp"
#include <string>

static void data_analytics_uint32_uint32_arguments(benchmark::internal::Benchmark* b) {
    for (int64_t threads = 1; threads <= 16; ++threads) {
        b->Args(get_uint32_benchmark_args_named(threads, 0, 1000000, 10000, 
            1, 10, 0, 0, 0, 0));
    }
}

START_BENCHMARK(std::string("data_analytics_uint32_uint32"), data_analytics_uint32_uint32_arguments)