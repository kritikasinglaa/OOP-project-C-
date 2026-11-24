cmake_minimum_required(VERSION 3.10)
project(ErlHub CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB LIB_SOURCES ${CMAKE_SOURCE_DIR}/src/*.cpp)
list(REMOVE_ITEM LIB_SOURCES ${CMAKE_SOURCE_DIR}/src/main.cpp)

add_library(erlhub STATIC ${LIB_SOURCES})
target_include_directories(erlhub PUBLIC ${CMAKE_SOURCE_DIR}/include)

add_executable(erl-hub src/main.cpp)
target_link_libraries(erl-hub PRIVATE erlhub)

add_executable(erl-tests tests/test_erlhub.cpp)
target_link_libraries(erl-tests PRIVATE erlhub)

if(MSVC)
  target_compile_options(erlhub PRIVATE /W4)
else()
  target_compile_options(erlhub PRIVATE -Wall -Wextra -Wpedantic)
endif()

enable_testing()
add_test(NAME erl-tests COMMAND erl-tests)
