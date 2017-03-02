cmake_minimum_required(VERSION 3.2)
project(xzbuf)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES
    include/xzbuf.hpp)

add_library(xzbuf INTERFACE)
target_sources(xzbuf
               INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include/xzbuf.hpp)
target_include_directories(xzbuf INTERFACE include)
target_link_libraries(xzbuf INTERFACE lzma)


add_executable(xzbuf-test src/test.cpp)
target_link_libraries(xzbuf-test xzbuf)