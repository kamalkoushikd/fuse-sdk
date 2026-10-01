include(FetchContent)

set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_Declare(googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.15.2
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(googletest)

add_executable(fuse_sdk_tests
    test_sdk.cpp
    test_transfer.cpp
    test_mux_transfer.cpp
    test_http.cpp
)
target_link_libraries(fuse_sdk_tests PRIVATE fuse::sdk fuse::http GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(fuse_sdk_tests)
