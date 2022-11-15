cmake_minimum_required(VERSION 3.22)

project(seqlock_concurrent_hash_map LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2")

add_executable(bench perf/benchmark_hash_map.cpp)

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

FetchContent_MakeAvailable(
  onetbb
  googletest
  googlebenchmark
  libcuckoo)

find_package(Boost 1.74.0) 

target_include_directories(bench PRIVATE "${CMAKE_CURRENT_LIST_DIR}/external/boost/sync/include/")

target_link_libraries(bench PRIVATE TBB::tbb benchmark::benchmark libcuckoo)