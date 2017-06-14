cmake_minimum_required(VERSION 3.2)
include(ExternalProject)
include(CMakePackageConfigHelpers)
project(shrinkwrap VERSION 1.0.0)

enable_testing()

set(CMAKE_CXX_STANDARD 11)

if (BUILD_SHARED_LIBS)
    set(LIBLZMA_LIB_NAME ${CMAKE_SHARED_LIBRARY_PREFIX}lzma${CMAKE_SHARED_LIBRARY_SUFFIX})
    set(ZLIB_LIB_NAME ${CMAKE_SHARED_LIBRARY_PREFIX}z${CMAKE_SHARED_LIBRARY_SUFFIX})
else()
    set(LIBLZMA_LIB_NAME ${CMAKE_STATIC_LIBRARY_PREFIX}lzma${CMAKE_STATIC_LIBRARY_SUFFIX})
    set(ZLIB_LIB_NAME ${CMAKE_STATIC_LIBRARY_PREFIX}z${CMAKE_STATIC_LIBRARY_SUFFIX})
endif()

find_library(LIBLZMA_LIBRARIES
             NAMES ${LIBLZMA_LIB_NAME})

find_library(ZLIB_LIBRARIES
             NAMES ${ZLIB_LIB_NAME})

if (NOT LIBLZMA_LIBRARIES)
    message(FATAL_ERROR "lzma library not found")
endif()

add_library(shrinkwrap INTERFACE)
target_sources(shrinkwrap INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/shrinkwrap/xz.hpp;${CMAKE_CURRENT_SOURCE_DIR}/include/shrinkwrap/gz.hpp>)
target_include_directories(shrinkwrap INTERFACE
                           $<INSTALL_INTERFACE:include>
                           $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_link_libraries(shrinkwrap INTERFACE ${LIBLZMA_LIBRARIES} ${ZLIB_LIBRARIES})

add_executable(shrinkwrap-test src/test.cpp)
target_link_libraries(shrinkwrap-test shrinkwrap)

add_test(xz_seek_test shrinkwrap-test xzseek)
add_test(xz_iterator_test shrinkwrap-test xziter)
add_test(gz_iterator_test shrinkwrap-test gziter)
add_test(bgz_seek_test shrinkwrap-test bgzseek)
add_test(bgz_iterator_test shrinkwrap-test bgziter)

install(DIRECTORY include/shrinkwrap DESTINATION include)
install(TARGETS shrinkwrap EXPORT ${PROJECT_NAME}-config
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)

install(EXPORT ${PROJECT_NAME}-config DESTINATION share/${PROJECT_NAME})
write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config-version.cmake COMPATIBILITY SameMajorVersion)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config-version.cmake DESTINATION share/${PROJECT_NAME})

export(EXPORT ${PROJECT_NAME}-config)