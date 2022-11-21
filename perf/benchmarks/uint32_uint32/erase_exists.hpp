#include "../../benchmark_hash_map_framework.hpp"

static void erase_exists_uint32_uint32_arguments(benchmark::internal::Benchmark* b) {
    for (int64_t threads = 1; threads <= 16; ++threads) {
        b->Args(get_uint32_benchmark_args_named(threads, 10000000, 10000000, 10000000,
            0, 0, 1, 1, 1, 1));
    }
}

START_BENCHMARK(std::string("erase_exists_uint32_uint32"), erase_exists_uint32_uint32_arguments)