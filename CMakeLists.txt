cmake_minimum_required(VERSION 3.16)

project(fuse_sdk
    VERSION 0.1.0
    DESCRIPTION "Fuse SDK, the ergonomic send/recv API and Python bindings, built on fuse-nw's protocol core"
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

# This repo carries no C++ sources of its own. fuse/sdk.h, transfer.hpp,
# mux_transfer.hpp, and the fuse::sdk target they build into all live in
# fuse-nw, as its own fuse_sdk CMake target. Pulling it in via FetchContent
# rather than duplicating any of it here is the whole point of this repo
# existing as a thin layer. Bump FUSE_NW_GIT_TAG to track a newer fuse-nw.
set(FUSE_NW_GIT_TAG "5cdb393411c61b1f40562d34b96d0dfb17397487" CACHE STRING
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

# fuse-nw's own install() rules (brought in above) cover fuse::fuse and
# fuse::proto but deliberately exclude fuse_sdk and its headers, since that
# repo's own release is protocol-only. This repo's release is the SDK, so it
# adds fuse_sdk, sdk.h, transfer.hpp and mux_transfer.hpp to the same install
# manifest here instead, pulling the headers straight out of the fetched
# fuse-nw source tree (fuse_nw_SOURCE_DIR) since no copy of them lives here.
include(GNUInstallDirs)
install(TARGETS fuse_sdk
    EXPORT fuse-sdk-targets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(FILES
    "${fuse_nw_SOURCE_DIR}/include/fuse/sdk.h"
    "${fuse_nw_SOURCE_DIR}/include/fuse/transfer.hpp"
    "${fuse_nw_SOURCE_DIR}/include/fuse/mux_transfer.hpp"
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/fuse
)

# fuse-nw skips its own fuse.pc generation when it's a subdirectory (see its
# CMakeLists.txt), leaving this to the consumer, exactly so this repo can
# generate the right one here, with -lfuse_sdk in the Libs line.
if(FUSE_WITH_CRYPTO)
    set(FUSE_SDK_PC_REQUIRES_PRIVATE "wolfssl")
else()
    set(FUSE_SDK_PC_REQUIRES_PRIVATE "")
endif()
install(CODE "
    file(READ \"${CMAKE_CURRENT_SOURCE_DIR}/cmake/fuse.pc.in\" _fuse_pc_content)
    string(REPLACE \"@CMAKE_INSTALL_PREFIX@\" \"\${CMAKE_INSTALL_PREFIX}\" _fuse_pc_content \"\${_fuse_pc_content}\")
    string(REPLACE \"@CMAKE_INSTALL_LIBDIR@\" \"${CMAKE_INSTALL_LIBDIR}\" _fuse_pc_content \"\${_fuse_pc_content}\")
    string(REPLACE \"@CMAKE_INSTALL_INCLUDEDIR@\" \"${CMAKE_INSTALL_INCLUDEDIR}\" _fuse_pc_content \"\${_fuse_pc_content}\")
    string(REPLACE \"@PROJECT_DESCRIPTION@\" \"${PROJECT_DESCRIPTION}\" _fuse_pc_content \"\${_fuse_pc_content}\")
    string(REPLACE \"@PROJECT_VERSION@\" \"${PROJECT_VERSION}\" _fuse_pc_content \"\${_fuse_pc_content}\")
    string(REPLACE \"@PROJECT_HOMEPAGE_URL@\" \"${PROJECT_HOMEPAGE_URL}\" _fuse_pc_content \"\${_fuse_pc_content}\")
    string(REPLACE \"@FUSE_PC_REQUIRES_PRIVATE@\" \"${FUSE_SDK_PC_REQUIRES_PRIVATE}\" _fuse_pc_content \"\${_fuse_pc_content}\")
    file(WRITE \"${CMAKE_CURRENT_BINARY_DIR}/fuse.pc\" \"\${_fuse_pc_content}\")
")
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/fuse.pc"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)

if(FUSE_SDK_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(FUSE_SDK_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
