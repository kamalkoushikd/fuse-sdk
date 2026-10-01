# Quickstart: the smallest complete send/receive pair using the public API.
add_executable(fuse_quickstart_recv quickstart_recv.cpp)
target_link_libraries(fuse_quickstart_recv PRIVATE fuse::sdk)

add_executable(fuse_quickstart_send quickstart_send.cpp)
target_link_libraries(fuse_quickstart_send PRIVATE fuse::sdk)

# A concurrent echo server/client using the socket-style API, plain C,
# exactly as an external developer would write them.
add_executable(fuse_echo_sdk_server echo_server.c)
target_link_libraries(fuse_echo_sdk_server PRIVATE fuse::sdk)

add_executable(fuse_echo_sdk_client echo_client.c)
target_link_libraries(fuse_echo_sdk_client PRIVATE fuse::sdk)

# libfuse_sdk.so/libfuse_proto.so are these executables' own direct runtime
# dependency, not just build-time, so $ORIGIN/@loader_path is needed for an
# install prefix where bin/ and lib/ are separate directories.
set_target_properties(
    fuse_quickstart_recv fuse_quickstart_send
    fuse_echo_sdk_server fuse_echo_sdk_client
    PROPERTIES
    INSTALL_RPATH "$<IF:$<PLATFORM_ID:Darwin>,@loader_path/../${CMAKE_INSTALL_LIBDIR},$ORIGIN/../${CMAKE_INSTALL_LIBDIR}>"
)
install(TARGETS fuse_quickstart_recv fuse_quickstart_send
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
