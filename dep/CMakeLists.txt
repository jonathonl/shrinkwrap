cmake_minimum_required(VERSION 3.2)
include(ExternalProject)
project(dependencies)
set(CMAKE_CXX_STANDARD 11)

ExternalProject_Add(xz_project
                    BUILD_IN_SOURCE 1
                    GIT_REPOSITORY "http://git.tukaani.org/xz.git"
                    GIT_TAG "v5.2.3"
                    UPDATE_COMMAND ""
                    CONFIGURE_COMMAND autoreconf -fi && ./configure --prefix=${CMAKE_INSTALL_PREFIX} #<INSTALL_DIR>
                    BUILD_COMMAND make
                    INSTALL_COMMAND make install)
