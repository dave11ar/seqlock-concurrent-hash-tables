add_executable(stress_checked stress_checked.cpp)
target_link_libraries(stress_checked PRIVATE test_utils)

add_executable(stress_unchecked stress_unchecked.cpp)
target_link_libraries(stress_unchecked PRIVATE test_utils)

add_test(NAME stress_checked COMMAND stress_checked)
add_test(NAME stress_unchecked COMMAND stress_unchecked)
