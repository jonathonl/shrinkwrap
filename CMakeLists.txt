cmake_minimum_required(VERSION 3.2)
include(ExternalProject)
include(CMakePackageConfigHelpers)
project(xzbuf VERSION 1.0.0)

enable_testing()

set(CMAKE_CXX_STANDARD 11)

#ExternalProject_Add(xz_project
#                    BUILD_IN_SOURCE 1
#                    GIT_REPOSITORY "http://git.tukaani.org/xz.git"
#                    GIT_TAG "v5.2.3"
#                    UPDATE_COMMAND ""
#                    CONFIGURE_COMMAND autoreconf -fi && ./configure --prefix=${CMAKE_CURRENT_BINARY_DIR} #<INSTALL_DIR>
#                    BUILD_COMMAND make debug
#                    INSTALL_COMMAND make install)


find_package(LibLZMA REQUIRED)

add_library(xzbuf INTERFACE)
target_sources(xzbuf INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/xzbuf.hpp>)
target_include_directories(xzbuf INTERFACE
                           $<INSTALL_INTERFACE:include>
                           $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                           ${LIBLZMA_INCLUDE_DIRS})
target_link_libraries(xzbuf INTERFACE ${LIBLZMA_LIBRARIES})



add_executable(xzbuf-seek-test src/test.cpp)
target_link_libraries(xzbuf-seek-test xzbuf)

add_test(seek_test xzbuf-seek-test) #${CMAKE_BINARY_DIR}/xzbuf-seek-test)

install(FILES $<TARGET_PROPERTY:xzbuf,INTERFACE_SOURCES> DESTINATION include)
install(TARGETS xzbuf EXPORT xzbuf-config
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)

install(EXPORT ${PROJECT_NAME}-config DESTINATION share/${PROJECT_NAME})
write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config-version.cmake COMPATIBILITY SameMajorVersion)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config-version.cmake DESTINATION share/${PROJECT_NAME})