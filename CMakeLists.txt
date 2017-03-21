cmake_minimum_required(VERSION 3.2)
include(ExternalProject)
include(CMakePackageConfigHelpers)
project(xzbuf VERSION 1.0.0)

enable_testing()

set(CMAKE_CXX_STANDARD 11)

if (${INSTALL_DEPENDENCIES})
    message("Installing dependencies ... ")
    file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/dep)
    execute_process(COMMAND ${CMAKE_COMMAND} ${CMAKE_SOURCE_DIR}/dep WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/dep)
    execute_process(COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR}/dep WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/dep RESULT_VARIABLE build_result)
    if(build_result)
        message(FATAL_ERROR "Dependency installation failed.")
    endif()
    message("Done installing dependencies ... ")

    set(LIBLZMA_INCLUDE_DIRS $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/dep/xz_project-prefix/src/xz_project/src/liblzma>)
    if (${BUILD_SHARED_LIBS})
        set(LIBLZMA_LIB_NAME ${CMAKE_SHARED_LIBRARY_PREFIX}lzma${CMAKE_SHARED_LIBRARY_SUFFIX})
    else()
        set(LIBLZMA_LIB_NAME ${CMAKE_STATIC_LIBRARY_PREFIX}lzma${CMAKE_STATIC_LIBRARY_SUFFIX})
    endif()

    find_library(LIBLZMA_LIBRARIES
                 NAMES ${LIBLZMA_LIB_NAME}
                 PATHS ${CMAKE_BINARY_DIR}/dep/xz_project-prefix/src/xz_project/src/liblzma/.libs
                 NO_DEFAULT_PATH
                 REQUIRED)
else()
    find_package(LibLZMA REQUIRED)
endif()



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

export(EXPORT ${PROJECT_NAME}-config)