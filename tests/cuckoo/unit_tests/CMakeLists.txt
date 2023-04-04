include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG release-1.12.1
)
FetchContent_MakeAvailable(googletest)

add_executable(unit_tests
    test_constructor.cpp
    test_hash_properties.cpp
    test_heterogeneous_compare.cpp
    test_iterator.cpp
    test_maximum_hashpower.cpp
    test_minimum_load_factor.cpp
    test_resize.cpp
    test_user_exceptions.cpp
    test_locked_table.cpp
    test_bucket_container.cpp
    unit_test_util.cpp
    unit_test_util.hpp
)

target_link_libraries(unit_tests PRIVATE GTest::gtest_main)
add_test(NAME unit_tests COMMAND unit_tests)
