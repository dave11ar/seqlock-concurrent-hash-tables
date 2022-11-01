cmake_minimum_required(VERSION 3.22)

project(seqlock_concurrent_hash_map LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

add_executable(bench perf/benchmark_hash_map.cpp)

include(FetchContent)
FetchContent_Declare(
  onetbb
  GIT_REPOSITORY https://github.com/oneapi-src/oneTBB.git
  GIT_TAG v2021.6.0
)
option(TBB_TEST OFF)
FetchContent_MakeAvailable(onetbb)

find_package(Boost 1.74.0) 

target_include_directories(bench PRIVATE "${CMAKE_CURRENT_LIST_DIR}/external/boost/sync/include/")
target_link_libraries(bench PRIVATE TBB::tbb)
