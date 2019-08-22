cmake_minimum_required(VERSION 3.6)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/out")

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/)
list(APPEND CMAKE_PREFIX_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/)

if (NOT TARGET gtest)
    add_subdirectory(googletest-release-1.8.1)
endif()

project(simpleecs)

find_package(SDL2)
find_package(OpenGL)
find_package(SDL2TTF)
find_package(SFML 2 COMPONENTS graphics window system)

IF( SDL2_FOUND AND OpenGL_FOUND AND SDL2TTF_FOUND )
    include_directories(${SDL2_INCLUDE_DIR} ${SDL2TTF_INCLUDE_DIR})
    add_executable(movingblocks code/examples/movingblocks/main.cpp)
    target_link_libraries(movingblocks ${SDL2_LIBRARIES} ${OPENGL_LIBRARIES} ${SDL2TTF_LIBRARY})
ELSE()
    message("Can't build example movingblocks: Install SDL2 OpenGL and SDL2TTF, if you wanna run it.")
ENDIF()

IF( SFML_FOUND )
    include_directories(${SFML_INCLUDE_DIR})
    add_executable(entitxExample code/examples/exampleFromEntityx/example.cc)
    target_link_libraries(entitxExample ${SFML_LIBRARIES})
ELSE()
    message("Can't build example entitxExample: Install SFML, if you wanna run it.")
ENDIF()

add_executable(walkingLetters code/examples/walkingLetters/main.cpp)

add_executable(benchmark_test code/test/Benchmarks_test.cc)
target_link_libraries(benchmark_test gtest gtest_main)

add_executable(functionality_test
        code/test/functionality/BitsetTest.cc
        code/test/functionality/CoreTest.cc
        code/test/functionality/RtEcsEventHandelerTest.cc)
target_link_libraries(functionality_test gtest gtest_main)