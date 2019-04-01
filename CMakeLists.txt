cmake_minimum_required(VERSION 3.13)
project(ECS)

add_subdirectory(googletest-release-1.8.1)

set(CMAKE_CXX_STANDARD 11)

add_executable(benchmark_test code/test/Benchmarks_test.cc)
add_executable(functionality_test code/test/functionality/Bitset.cc code/test/functionality/Manager.cc)

target_link_libraries(benchmark_test gtest gtest_main)
target_link_libraries(functionality_test gtest gtest_main)