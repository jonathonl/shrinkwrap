cmake_minimum_required(VERSION 3.2)
include(ExternalProject)
include(CMakePackageConfigHelpers)
project(xzbuf VERSION 1.0.0)

enable_testing()

set(CMAKE_CXX_STANDARD 11)

if (${INSTALL_DEPENDENCIES})
    message("Installing dependencies ... ")
    file(MAKE_DIRECTORY dep/build)
    execute_process(COMMAND ${CMAKE_COMMAND} -DCMAKE_INSTALL_PREFIX=./ .. WORKING_DIRECTORY "dep/build")
    execute_process(COMMAND make WORKING_DIRECTORY "dep/build" RESULT_VARIABLE build_result)
    if(result)
        message(FATAL_ERROR "Dependency installation failed.")
    endif()
    message("Done nstalling dependencies ... ")
endif()


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