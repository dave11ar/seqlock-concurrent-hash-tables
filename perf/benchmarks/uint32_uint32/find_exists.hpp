#include "../../benchmark_hash_map_framework.hpp"

static void find_exists_uint32_uint32_arguments(benchmark::internal::Benchmark* b) {
    for (int64_t threads = 1; threads <= 16; ++threads) {
        b->Args(get_uint32_benchmark_args_named(threads, 100000, 5000000, 10000,
            1, 0, 0, 1, 0, 0));
    }
}

START_BENCHMARK(std::string("find_exists_uint32_uint32"), find_exists_uint32_uint32_arguments)