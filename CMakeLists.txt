cmake_minimum_required(VERSION 3.2)
include(ExternalProject)
include(CMakePackageConfigHelpers)
project(shrinkwrap VERSION 1.0.0)

enable_testing()

set(CMAKE_CXX_STANDARD 11)

if (BUILD_SHARED_LIBS)
    set(LIBLZMA_LIB_NAME ${CMAKE_SHARED_LIBRARY_PREFIX}lzma${CMAKE_SHARED_LIBRARY_SUFFIX})
else()
    set(LIBLZMA_LIB_NAME ${CMAKE_STATIC_LIBRARY_PREFIX}lzma${CMAKE_STATIC_LIBRARY_SUFFIX})
endif()

find_library(LIBLZMA_LIBRARIES
             NAMES ${LIBLZMA_LIB_NAME})

if (NOT LIBLZMA_LIBRARIES)
    message(FATAL_ERROR "lzma library not found")
endif()

add_library(xzbuf INTERFACE)
target_sources(xzbuf INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/shrinkwrap/xzbuf.hpp>)
target_include_directories(xzbuf INTERFACE
                           $<INSTALL_INTERFACE:include>
                           $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_link_libraries(xzbuf INTERFACE ${LIBLZMA_LIBRARIES})

add_executable(xzbuf-test src/test.cpp)
target_link_libraries(xzbuf-test xzbuf)

add_test(seek_test xzbuf-test seek)
add_test(iterator_test xzbuf-test iterator)

install(DIRECTORY include/shrinkwrap DESTINATION include)
install(TARGETS xzbuf EXPORT ${PROJECT_NAME}-config
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)

install(EXPORT ${PROJECT_NAME}-config DESTINATION share/${PROJECT_NAME})
write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config-version.cmake COMPATIBILITY SameMajorVersion)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config-version.cmake DESTINATION share/${PROJECT_NAME})

export(EXPORT ${PROJECT_NAME}-config)