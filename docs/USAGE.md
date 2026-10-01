# Moving whole files or large buffers

`fuse/transfer.hpp` is a separate, higher-throughput API from `fuse/sdk.h`
(see [docs/SDK.md](SDK.md) for the socket-style server/client API). Use this
one for moving one big buffer or file fast, sharded across several UDP lanes.

## Install

```sh
curl -fsSL https://github.com/kamalkoushikd/fuse-sdk/releases/latest/download/install.sh | sh
```

or build from source: see the main [README.md](../README.md).

## Use it, CMake

```cmake
find_package(fuse CONFIG REQUIRED)

add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE fuse::sdk)
```

## Use it, pkg-config (no CMake)

```sh
g++ -std=c++17 main.cpp -o my_app $(pkg-config --cflags --libs fuse)
```

That is sufficient even for a non-system prefix: the `.pc` file carries an
rpath, so the resulting binary runs without `LD_LIBRARY_PATH`.

## Send something

Two calls. Start the receiver first, it must be bound before the sender's
opening message arrives.

**Receiver**

```cpp
#include <fuse/transfer.hpp>

fuse::TransferConfig cfg;
cfg.bind_address = "0.0.0.0";
cfg.base_port    = 4433;
cfg.lanes        = 4;

std::vector<uint8_t> data;
auto status = fuse::receive_buffer(cfg, &data);
if (status != fuse::TransferStatus::Ok) {
    std::fprintf(stderr, "receive failed: %s\n", fuse::to_string(status));
}
```

**Sender**

```cpp
#include <fuse/transfer.hpp>

fuse::TransferConfig cfg;
cfg.host      = "192.0.2.10";
cfg.base_port = 4433;
cfg.lanes     = 4;          // must match the receiver

fuse::TransferStats stats;
auto status = fuse::send_buffer(cfg, data.data(), data.size(), &stats);
std::printf("%.1f MB/s\n", stats.throughput_mb_per_s());
```

Files, if that is what you have:

```cpp
fuse::send_file(cfg, "/path/to/input");
fuse::receive_file(cfg, "/path/to/output");
```

Both calls block until the transfer completes or fails. Everything
underneath, sharding, batched syscalls, adaptive block sizing,
retransmission, congestion control, is handled for you.

`receive_file` writes blocks to `<output>.part` as they arrive and renames it
to `<output>` only once the whole file is in and synced, so `<output>` is
never left half-written.

**Resuming an interrupted transfer.** If a file transfer stops part-way
(Ctrl+C, a dropped connection, a crash on either side), the receiver keeps
`<output>.part` together with a record of what it already has. Run the same
`send_file` and `receive_file` again, same input file, same output path,
and only the missing parts are sent; the whole file is then checked against
a digest from the sender before it is put in place. If the input file
changed in the meantime (name, size or modification time), the transfer
starts over automatically. To force a fresh start, delete `<output>.part`.
`send_buffer`/`receive_buffer` don't resume.

## Encryption

Set a pre-shared key on **both** ends:

```cpp
cfg.pre_shared_key = "a-key-both-sides-share";
```

This gives AES-256-GCM confidentiality and integrity, with a key derived
freshly per session. Cost is roughly 18% throughput.

> **It does not provide forward secrecy.** The key comes from your PSK plus
> a per-session salt, so someone who records the traffic and later obtains
> the PSK can decrypt it. If that is part of your threat model, use the DTLS
> path in `fuse/proto/dtls.hpp` (in the fuse-nw protocol core), which
> negotiates ECDHE-PSK, at the cost of the batching that makes the fast path
> fast.
>
> A build without the crypto backend returns `TransferStatus::Unsupported`
> rather than sending in the clear. Check `fuse::encryption_available()`.

## Ports and firewalls

A transfer uses **`lanes` consecutive UDP ports** starting at `base_port`:
lane *i* uses `base_port + i`. With the default of 4 lanes and port 4433,
open UDP **4433-4436** on the receiver. Both ends must agree on `base_port`
and `lanes`.

## Choosing `lanes`

More lanes raise the achievable packet rate only until the CPU saturates;
past that, throughput actively **declines** as threads contend.

A lane costs roughly **2.7 cores**, one sender thread, one receiver thread,
and the kernel's UDP/GSO/copy work. So as a starting estimate:

    lanes = cores / 3            # sender and receiver on different machines
    lanes = cores / 6            # both endpoints on the same machine

On a 16-core host with both ends local, throughput peaked at **6 lanes**
(4255 MB/s) and fell to 3034 MB/s at 16 lanes. Restricted to 8 cores, the
peak moved to 4 lanes, the knee tracks available CPU, so measure on your own
hardware rather than trusting a fixed number.

## Errors

`TransferStatus` values and what they usually mean:

| status | typical cause |
|---|---|
| `Ok` | transfer completed, every byte delivered |
| `ConfigError` | `lanes == 0`, bad port, unreadable file |
| `SocketError` | port already bound, or the port range is blocked |
| `Timeout` | peer never started, went away, or the link stalled |
| `Incomplete` | finished without delivering every byte |
| `Unsupported` | encryption requested from a build without crypto |
| `Cancelled` | `TransferConfig::cancel` was set on this side (e.g. Ctrl+C) |
| `PeerAborted` | the other side cancelled or gave up, and said so |
| `IoError` | the receiver couldn't write its output (disk full, permissions) |
| `VerifyFailed` | a resumed file didn't match the sender's digest; the partial copy was discarded, so running again starts fresh |
| `ResourceLimit` | the sender claimed a size the receiver couldn't allocate |

`fuse::to_string(status)` gives a printable form.

## Runnable examples

```sh
# terminal 1
fuse_quickstart_recv 0.0.0.0 4433 /tmp/out.bin 4 [optional-key]
# terminal 2
fuse_quickstart_send 127.0.0.1 4433 /tmp/in.bin 4 [optional-key]
```

Source: `examples/quickstart_recv.cpp`, `examples/quickstart_send.cpp`.

## Going lower-level

`fuse/transfer.hpp` is built from the protocol core's own public pieces, in
[fuse-nw](https://github.com/kamalkoushikd/fuse-nw):

| header | what it gives you |
|---|---|
| `fuse/proto/udp.hpp` | UDP socket with GSO batching and `recvmmsg` |
| `fuse/proto/block.hpp` | block header encode/decode |
| `fuse/proto/registry.hpp` | sender-side retransmit registry |
| `fuse/proto/receiver.hpp` | receive window, O(1) loss detection |
| `fuse/proto/congestion.hpp` | per-stream AIMD controller |
| `fuse/proto/session_crypto.hpp` | session keys + per-lane AEAD |
| `fuse/proto/orchestrator.hpp` | autoscaling worker pool |
| `fuse/proto/setup.hpp` | SETUP handshake |

Measured performance and its caveats:
[fuse-nw's bench/RESULTS.md](https://github.com/kamalkoushikd/fuse-nw/blob/main/bench/RESULTS.md).
