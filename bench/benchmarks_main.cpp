#include <cstring>

#include "benchmarks/uint32_uint32/erase_exists.hpp"
#include "benchmarks/uint32_uint32/find_exists.hpp"
#include "benchmarks/uint32_uint32/find_modify_eq.hpp"
#include "benchmarks/uint32_uint32/insert_or_assigns.hpp"
#include "benchmarks/uint32_uint32/inserts.hpp"
#include "benchmarks/uint32_uint32/random.hpp"

bool ParseInt32Flag(const char* arg, const char* flag, int32_t& value) {
  const size_t flag_len = strlen(flag);
  if (strncmp(flag, arg + 2, flag_len) != 0) {
    return false;
  }

  value = atoi(arg + (2 + flag_len + 1));
  return true;
}

int main(int argc, char** argv) {
  char arg0_default[] = "benchmark";
  char* args_default = arg0_default;
  if (!argv) {
    argc = 1;
    argv = &args_default;
  }
  benchmark::Initialize(&argc, argv);

  bool unrecognized = false;
  for (int i = 1; i < argc; ++i) {
    if (!ParseInt32Flag(argv[i], "benchmark_threads_step", threads_step) &&
        !ParseInt32Flag(argv[i], "benchmark_threads_count", threads_count)) {
      fprintf(stderr, "%s: error: unrecognized command-line flag: %s\n", argv[0],
        argv[i]);
      unrecognized = true;
    }
  }

  if (unrecognized) {
    return 1;
  }

#ifdef UIN32_UINT32_BENCHMARK_ERASE_EXISTS
  START_BENCHMARK(std::string("erase_exists_uint32_uint32"), erase_exists_uint32_uint32_arguments)
#endif
#ifdef UIN32_UINT32_BENCHMARK_FIND_EXISTS
  START_BENCHMARK(std::string("find_exists_uint32_uint32"), find_exists_uint32_uint32_arguments)
  START_BENCHMARK(std::string("find_exists_high_contentions_uint32_uint32"), find_exists_high_contentions_uint32_uint32_arguments)
#endif
#ifdef UIN32_UINT32_BENCHMARK_FIND_MODIFY_EQ
  START_BENCHMARK(std::string("find_modify_eq_uint32_uint32"), find_modify_eq_uint32_uint32_arguments)
  START_BENCHMARK(std::string("find_modify_eq_high_contention_uint32_uint32"), find_modify_eq_high_contention_uint32_uint32_arguments)
#endif
#ifdef UIN32_UINT32_BENCHMARK_INSERT_OR_ASSIGNS
  START_BENCHMARK(std::string("insert_or_assigns_uint32_uint32"), insert_or_assigns_uint32_uint32_arguments)
  START_BENCHMARK(std::string("insert_or_assigns_high_contention_uint32_uint32"), insert_or_assigns_high_contention_uint32_uint32_arguments)
#endif
#ifdef UIN32_UINT32_BENCHMARK_INSERTS
  START_BENCHMARK(std::string("inserts_rehashing_uint32_uint32"), inserts_rehashing_uint32_uint32_arguments)
  START_BENCHMARK(std::string("inserts_no_rehashing_uint32_uint32"), inserts_no_rehashing_uint32_uint32_arguments)
#endif
#ifdef UIN32_UINT32_BENCHMARK_RANDOM
  START_BENCHMARK(std::string("random_uint32_uint32"), random_uint32_uint32_arguments)
  START_BENCHMARK(std::string("random_high_contention_uint32_uint32"), random_high_contention_uint32_uint32_arguments)
#endif

  benchmark::RunSpecifiedBenchmarks();
  benchmark::Shutdown();

  return 0;
}
