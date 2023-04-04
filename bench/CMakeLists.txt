add_executable(bench benchmarks_main.cpp)

include(FetchContent)
FetchContent_Declare(
  onetbb
  GIT_REPOSITORY https://github.com/oneapi-src/oneTBB.git
  GIT_TAG v2021.6.0
)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG release-1.12.1
)
FetchContent_Declare(
  googlebenchmark
  GIT_REPOSITORY https://github.com/google/benchmark.git
  GIT_TAG v1.7.1
)
FetchContent_Declare(
  libcuckoo
  GIT_REPOSITORY https://github.com/efficient/libcuckoo.git
  GIT_TAG v0.3.1
)
option(TBB_TEST OFF)

FetchContent_MakeAvailable(
  onetbb
  googletest
  googlebenchmark
  libcuckoo
)

target_link_libraries(bench PRIVATE TBB::tbb libcuckoo benchmark::benchmark)
target_include_directories(bench PRIVATE include)
target_compile_definitions(bench PRIVATE
  UIN32_UINT32_BENCHMARK_DATA_ANALYTICS
  UIN32_UINT32_BENCHMARK_ERASE_EXISTS
  UIN32_UINT32_BENCHMARK_FIND_EXISTS_HIGH_CONTENTION
  UIN32_UINT32_BENCHMARK_FIND_EXISTS
  UIN32_UINT32_BENCHMARK_FIND_MODIFY_EQ_HIGH_CONTENTION
  UIN32_UINT32_BENCHMARK_FIND_MODIFY_EQ
  UIN32_UINT32_BENCHMARK_FIND_UPSERT_HIGH_CONTENTION
  UIN32_UINT32_BENCHMARK_FIND_UPSERT
  UIN32_UINT32_BENCHMARK_RANDOM
)