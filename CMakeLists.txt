cmake_minimum_required(VERSION 3.6)
cmake_policy(SET CMP0072 NEW)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/out")

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/)
list(APPEND CMAKE_PREFIX_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/)

project(SimpleECS)

add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(${PROJECT_NAME} INTERFACE code)

if (NOT TARGET gtest)
    add_subdirectory(libs/googletest)
endif()

find_package(SDL2)
find_package(OpenGL)
find_package(SDL2TTF)
find_package(SFML 2 COMPONENTS graphics window system)

IF( SDL2_FOUND AND OpenGL_FOUND AND SDL2TTF_FOUND )
    include_directories(${SDL2_INCLUDE_DIR} ${SDL2TTF_INCLUDE_DIR})
    add_executable(movingblocks examples/movingblocks/main.cpp)
    target_link_libraries(movingblocks ${SDL2_LIBRARIES} ${OPENGL_LIBRARIES} ${SDL2TTF_LIBRARY} ${PROJECT_NAME})
ELSE()
    message("Can't build example movingblocks: Install SDL2 OpenGL and SDL2TTF, if you wanna run it.")
ENDIF()

IF( SFML_FOUND )
    include_directories(${SFML_INCLUDE_DIR})
    add_executable(entitxExample examples/exampleFromEntityx/example.cc)
    target_link_libraries(entitxExample ${SFML_LIBRARIES} ${PROJECT_NAME})
ELSE()
    message("Can't build example entitxExample: Install SFML, if you wanna run it.")
ENDIF()

add_executable(walkingLetters examples/walkingLetters/main.cpp)
target_link_libraries(walkingLetters ${PROJECT_NAME})

add_executable(benchmark_test test/Benchmarks_test.cc)
target_link_libraries(benchmark_test gtest gtest_main ${PROJECT_NAME})

add_executable(functionality_test
        test/functionality/Functionality_Tests.cc)
target_link_libraries(functionality_test gtest gtest_main ${PROJECT_NAME})