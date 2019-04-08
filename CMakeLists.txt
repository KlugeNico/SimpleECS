cmake_minimum_required(VERSION 3.13)
project(ECS)
project(clickblocks)

add_subdirectory(googletest-release-1.8.1)

set(CMAKE_CXX_STANDARD 11)

find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)

add_executable(clickblocks code/example/clickblocks/main.cpp)
target_link_libraries(clickblocks ${SDL2_LIBRARIES} ${OPENGL_LIBRARIES} )

add_executable(benchmark_test code/test/Benchmarks_test.cc)
add_executable(functionality_test code/test/functionality/Bitset.cc code/test/functionality/Core.cc code/test/functionality/RealTimeEcs.cc)

target_link_libraries(benchmark_test gtest gtest_main)
target_link_libraries(functionality_test gtest gtest_main)