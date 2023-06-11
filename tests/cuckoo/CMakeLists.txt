add_library(test_utils INTERFACE)
target_include_directories(test_utils INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)

option (CUCKOO_SEQLOCK_BUILD_TESTS "build all cuckoo tests" ON)
option (CUCKOO_SEQLOCK_BUILD_STRESS_TESTS "build the stress tests" ON)
option (CUCKOO_SEQLOCK_BUILD_UNIT_TESTS "build the unit tests" ON)

if (CUCKOO_SEQLOCK_BUILD_TESTS OR CUCKOO_SEQLOCK_BUILD_UNIT_TESTS)
    add_subdirectory(unit_tests)
endif()

if (CUCKOO_SEQLOCK_BUILD_TESTS OR CUCKOO_SEQLOCK_BUILD_STRESS_TESTS)
    add_subdirectory(stress_tests)
endif()
