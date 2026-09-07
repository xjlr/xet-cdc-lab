# xet-cdc-lab

`xet-cdc-lab` is a small C++20 systems project that reproduces and explores the
content-defined chunking (CDC) layer described by the Hugging Face Xet protocol.

The goal is not to build a complete Xet client. The project focuses on one narrow,
verifiable question:

> Can an independent C++ implementation produce exactly the same chunk boundaries
> and chunk hashes as the published Xet reference data?

## Planned commands

```text
xet-cdc chunk <file>
xet-cdc validate <file> <reference.chunks>
xet-cdc compare <original> <modified>
```

- `chunk` will print offsets and sizes produced by the GearHash-based CDC algorithm.
- `validate` will compare locally computed chunks with Hugging Face's reference manifest.
- `compare` will report chunk and byte reuse between two versions of a file.

The CLI currently exposes the command structure only. The algorithm will be added in
small, independently tested steps.

## Scope

### Phase 1: exact chunk boundaries

- implement the fixed 256-entry GearHash table;
- use 64-bit wrapping arithmetic;
- enforce the 8 KiB minimum and 128 KiB maximum chunk sizes, noting that the
  minimum does not apply to the final chunk or a file smaller than the minimum;
- reset the rolling state after each emitted boundary;
- support input incrementally so results do not depend on read-buffer size;
- validate all 796 boundaries of the official reference CSV.

### Phase 2: Xet chunk hashes

- compute the protocol's keyed BLAKE3 chunk hash;
- parse the published `<hash> <length>` manifest;
- validate both chunk length and hash;
- document the hash-string byte-order conversion explicitly.

### Phase 3: deduplication experiment

- generate deterministic test data;
- apply replacement, insertion, and deletion edits;
- compare content hashes between the original and modified versions;
- report reused chunks, new chunks, reused bytes, and reuse percentage.

## Benchmarks

The validation command performs the full pipeline:

1. content-defined chunking;
2. keyed BLAKE3 hashing;
3. Xet hash conversion;
4. comparison against the published reference manifest.

The reference file contains **796 chunks**.

### Debug vs Release

Debug build:

```text
Validation successful: 796 chunks matched.

real    0m4.599s
user    0m4.582s
sys     0m0.016s
```

Release build:

```text
Validation successful: 796 chunks matched.

real    0m0.415s
user    0m0.395s
sys     0m0.020s
```

The Release build is roughly **11x faster** than the Debug build.

### Release benchmark stability

Ten consecutive Release runs produced:

```text
0.41
0.41
0.41
0.42
0.41
0.41
0.41
0.40
0.43
0.44
```

Summary:

- median runtime: **~0.41 s**;
- observed range: **0.40-0.44 s**;
- validated chunks: **796**.

The timings are stable across repeated runs.

### CDC reuse experiments

Small edits were applied to a ~63.5 MB CSV file and compared against the original
using chunk hashes.

#### 1-byte deletion in the middle

```text
Original chunks: 796
Modified chunks: 796
Reused chunks:   794
New chunks:      2

Original bytes:  63527244
Modified bytes:  63527243
Reused bytes:    63327129
New bytes:       200114
Reuse ratio:     99.68%
```

#### 1-byte insertion in the middle

```text
Original chunks: 796
Modified chunks: 796
Reused chunks:   795
New chunks:      1

Original bytes:  63527244
Modified bytes:  63527245
Reused bytes:    63436649
New bytes:       90596
Reuse ratio:     99.86%
```

#### `"hello world"` inserted at the beginning

```text
Original chunks: 796
Modified chunks: 796
Reused chunks:   794
New chunks:      2

Original bytes:  63527244
Modified bytes:  63527258
Reused bytes:    63290073
New bytes:       237185
Reuse ratio:     99.63%
```

These experiments show the expected CDC behavior: local edits disturb only a small
region of chunk boundaries, after which the chunker resynchronizes and most later
chunks remain reusable.

## Design

The intended dependency direction is:

```text
CLI -> validation / comparison -> chunk hashing -> CDC boundary detector
```

The core CDC code will not depend on filesystem or command-line concerns. Its state will
survive arbitrary input-buffer boundaries, which lets tests feed identical data using
different block sizes and verify streaming determinism.

Planned core types:

- `ChunkBoundary`: byte offset and length;
- `GearHash`: the protocol's 64-bit rolling state;
- `Chunker`: consumes byte spans and emits boundaries;
- `ChunkHash`: a strongly typed 32-byte hash;
- `ReferenceManifest`: parser for Hugging Face's `.chunks` format;
- `DedupReport`: reuse statistics for two chunk sequences.

## Build

Requirements:

- C++20 compiler (GCC 11+, Clang 14+, or equivalent);
- CMake 3.21+.

```bash
cmake --preset debug
cmake --build --preset debug --parallel
ctest --preset debug
./build/debug/xet-cdc --help
```

The `release` and `asan` presets provide a release build and a Debug build with
AddressSanitizer and UndefinedBehaviorSanitizer. The test suite uses Catch2,
which CMake downloads and verifies by SHA-256 during the first configuration.

On Ubuntu 22.04, install the build tools if necessary:

```bash
sudo apt update
sudo apt install build-essential cmake
```

## Reference material

- [Xet protocol specification](https://huggingface.co/docs/xet/index)
- [Content-defined chunking algorithm](https://huggingface.co/docs/xet/chunking)
- [Hashing methods](https://huggingface.co/docs/xet/hashing)
- [Official reference files](https://huggingface.co/datasets/xet-team/xet-spec-reference-files)

Reference data should be downloaded into `reference-data/`; that directory is ignored by
Git because the upstream Hugging Face repository is the source of truth.

## Non-goals

- implementing Xorbs, shards, reconstruction, authentication, or CAS networking;
- competing with the optimized Rust implementation in `xet-core`;
- claiming protocol compatibility before every reference case passes.
