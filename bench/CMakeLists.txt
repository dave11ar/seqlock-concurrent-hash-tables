add_executable(bench benchmarks_main.cpp)

include(FetchContent)
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

FetchContent_MakeAvailable(
  googletest
  googlebenchmark
  libcuckoo
)

target_link_libraries(bench PRIVATE libcuckoo benchmark::benchmark)
target_include_directories(bench PRIVATE include)
target_compile_definitions(bench PRIVATE
  UIN32_UINT32_BENCHMARK_ERASE_EXISTS
  UIN32_UINT32_BENCHMARK_FIND_EXISTS
  UIN32_UINT32_BENCHMARK_FIND_MODIFY_EQ
  UIN32_UINT32_BENCHMARK_INSERT_OR_ASSIGNS
  UIN32_UINT32_BENCHMARK_INSERTS
  UIN32_UINT32_BENCHMARK_RANDOM
)
