cmake_minimum_required(VERSION 3.2)
project(xzbuf)

set(CMAKE_CXX_STANDARD 11)

include_directories(include)

set(SOURCE_FILES
    include/xzbuf.hpp
    src/test.cpp src/test.cpp)

add_executable(xzbuf-test ${SOURCE_FILES})
target_link_libraries(xzbuf-test lzma)