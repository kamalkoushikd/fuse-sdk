# fuse-sdk

The ergonomic SDK layer for [Fuse](https://github.com/kamalkoushikd/fuse-nw):
socket-style `listen`/`accept`/`connect`/`send`/`recv` (`fuse/sdk.h`), the
higher-throughput transfer APIs (`fuse/transfer.hpp`, `fuse/mux_transfer.hpp`),
HTTP-over-Fuse (`fuse/http.h`), and the Python bindings. Split out of fuse-nw
so the wire protocol (block layer, congestion control, handshake, ...) and
the application-facing API can evolve and version independently.

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

## HTTP over Fuse

```c
#include <fuse/http.h>

fuse_http_request req = {"GET", "/", "", nullptr, 0};
fuse_http_response resp;
fuse_http_fetch(&cfg, &req, &resp, 5000);
printf("%d %s\n", resp.status, resp.reason);
fuse_http_response_free(&resp);
```

One request, one response, one Fuse message each way (no keep-alive or
pipelining yet, each connection is good for exactly one exchange). Discovery
is Alt-Svc-style, token `hfuse-00`, versioning this mapping independently of
the underlying wire protocol. See `fuse/http.h`'s own comments (in
[fuse-nw](https://github.com/kamalkoushikd/fuse-nw)) for the exact wire
format and the reasoning behind it.

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

`pip install fuse-sdk` is live on PyPI (both x86_64 and aarch64 wheels, plus
an sdist). CI (`.github/workflows/publish-python.yml`) builds and publishes
on a version tag.

## Status

Split out of fuse-nw on 2026-10-01; fuse-nw's own release is now
protocol-only. This repo has its own tarball release and PyPI package
(`fuse-sdk`, replacing fuse-nw's old `fuse-transport`).
