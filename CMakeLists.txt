cmake_minimum_required(VERSION 3.2)
project(xzbuf)

set(CMAKE_CXX_STANDARD 11)

if(EXISTS ${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
    include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
    conan_basic_setup()
endif()

find_package(LibLZMA REQUIRED)
message("xz include dir: ${LIBLZMA_INCLUDE_DIRS}")
message("xz libs: ${LIBLZMA_LIBRARIES}")

add_library(xzbuf INTERFACE)
target_sources(xzbuf INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include/xzbuf.hpp)
target_include_directories(xzbuf INTERFACE include ${LIBLZMA_INCLUDE_DIRS})
target_link_libraries(xzbuf INTERFACE ${LIBLZMA_LIBRARIES})
get_target_property(XZBUF_HEADER_FILES xzbuf INTERFACE_SOURCES)


add_executable(xzbuf-test src/test.cpp)
target_link_libraries(xzbuf-test xzbuf)

install(FILES "${XZBUF_HEADER_FILES}" DESTINATION include)