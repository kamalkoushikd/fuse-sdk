cmake_minimum_required(VERSION 3.16)

project(fuse_sdk
    VERSION 0.1.0
    DESCRIPTION "Fuse SDK: ergonomic send/recv API and Python bindings, built on fuse-nw's protocol core"
    HOMEPAGE_URL "https://github.com/kamalkoushikd/fuse-sdk"
    LANGUAGES C CXX
)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(FUSE_SDK_IS_TOP_LEVEL ON)
else()
    set(FUSE_SDK_IS_TOP_LEVEL OFF)
endif()

option(FUSE_SDK_BUILD_EXAMPLES "Build fuse-sdk example programs" ${FUSE_SDK_IS_TOP_LEVEL})
option(FUSE_SDK_BUILD_TESTS "Build the fuse-sdk test suite" ${FUSE_SDK_IS_TOP_LEVEL})

# This repo carries no C++ sources of its own: fuse/sdk.h, transfer.hpp,
# mux_transfer.hpp, and the fuse::sdk target they build into all live in
# fuse-nw (as of its own fuse_sdk CMake target). Pulling it in via
# FetchContent rather than duplicating any of it here is the whole point of
# this repo existing as a thin layer — bump FUSE_NW_GIT_TAG to track a
# newer fuse-nw.
set(FUSE_NW_GIT_TAG "6b2b53c5f1275651cc2c5b3557440a22b470b53d" CACHE STRING
    "fuse-nw commit/tag this SDK is built against")

include(FetchContent)
set(FUSE_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(FUSE_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(FUSE_BUILD_BENCH OFF CACHE BOOL "" FORCE)
FetchContent_Declare(fuse_nw
    GIT_REPOSITORY https://github.com/kamalkoushikd/fuse-nw.git
    GIT_TAG ${FUSE_NW_GIT_TAG}
)
FetchContent_MakeAvailable(fuse_nw)

if(FUSE_SDK_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(FUSE_SDK_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
