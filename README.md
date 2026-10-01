# fuse-sdk

The ergonomic SDK layer for [Fuse](https://github.com/kamalkoushikd/fuse-nw):
socket-style `listen`/`accept`/`connect`/`send`/`recv` (`fuse/sdk.h`), the
higher-throughput transfer APIs (`fuse/transfer.hpp`, `fuse/mux_transfer.hpp`),
and the Python bindings — split out of fuse-nw so the wire protocol (block
layer, congestion control, handshake, ...) and the application-facing API can
evolve and version independently.

This repo does not duplicate the protocol: `CMakeLists.txt` pulls fuse-nw in
via `FetchContent`, pinned to a tracked commit (`FUSE_NW_GIT_TAG`), and links
against it.

## Build

```sh
cmake -S . -B build
cmake --build build
```

Produces `libfuse_sdk` (linked against fuse-nw's `fuse::proto`), the
quickstart/echo example binaries, and the test suite (`FUSE_SDK_BUILD_TESTS`,
`FUSE_SDK_BUILD_EXAMPLES`, both on by default for a top-level build).

## Python

```sh
pip install fuse-sdk
```

See [`python/README.md`](python/README.md) for the API. CI
(`.github/workflows/publish-python.yml`) builds and can publish wheels, but
publishing is inert until a PyPI trusted publisher is configured for this
repo (see the workflow file's header comment) — not done yet.

## Status

Freshly split out of fuse-nw (2026-10-01). fuse-nw still carries its own copy
of this layer for now, so its existing `fuse-transport` PyPI package and
`install.sh` keep working unchanged — cutting those over to this repo is a
separate, later step.
