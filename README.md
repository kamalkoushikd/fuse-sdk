# fuse-sdk

The ergonomic SDK layer for [Fuse](https://github.com/kamalkoushikd/fuse-nw):
socket-style `listen`/`accept`/`connect`/`send`/`recv` (`fuse/sdk.h`), the
higher-throughput transfer APIs (`fuse/transfer.hpp`, `fuse/mux_transfer.hpp`),
and the Python bindings. Split out of fuse-nw so the wire protocol (block
layer, congestion control, handshake, ...) and the application-facing API can
evolve and version independently.

This repo does not duplicate the protocol: `CMakeLists.txt` pulls fuse-nw in
via `FetchContent`, pinned to a tracked commit (`FUSE_NW_GIT_TAG`), and links
against it.

## Install

```sh
curl -fsSL https://github.com/kamalkoushikd/fuse-sdk/releases/latest/download/install.sh | sh
```

or Python only:

```sh
pip install fuse-sdk
```

Full guide: [`docs/SDK.md`](docs/SDK.md) (socket-style API), or
[`docs/USAGE.md`](docs/USAGE.md) for the higher-throughput buffer/file
transfer API.

## Build from source

```sh
cmake -S . -B build
cmake --build build
```

Produces `libfuse_sdk` (linked against fuse-nw's `fuse::proto`), the
quickstart/echo example binaries, and the test suite (`FUSE_SDK_BUILD_TESTS`,
`FUSE_SDK_BUILD_EXAMPLES`, both on by default for a top-level build).

`scripts/make_release.sh [version]` builds the same relocatable tarball the
GitHub release ships.

## Python packaging

CI (`.github/workflows/publish-python.yml`) builds and can publish wheels,
but publishing is inert until a PyPI trusted publisher is configured for this
repo (see the workflow file's header comment), not done yet.

## Status

Split out of fuse-nw on 2026-10-01; fuse-nw's own release is now
protocol-only. This repo has its own tarball release and PyPI package
(`fuse-sdk`, replacing fuse-nw's old `fuse-transport`).
