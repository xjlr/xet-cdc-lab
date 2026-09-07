# xet-cdc-lab

[![CI](https://github.com/xjlr/xet-cdc-lab/actions/workflows/ci.yml/badge.svg)](https://github.com/xjlr/xet-cdc-lab/actions/workflows/ci.yml)

`xet-cdc-lab` is a small C++20 systems project that reproduces and explores the
content-defined chunking (CDC) and chunk-hashing layers described by the Hugging Face Xet
protocol.

The goal is not to build a complete Xet client. The project focuses on one narrow,
verifiable question:

> Can an independent C++ implementation produce exactly the same chunk boundaries
> and chunk hashes as the published Xet reference data?

**Status:** CDC boundaries, Xet chunk hashing, official reference validation, and
chunk-level deduplication experiments are implemented. The official reference CSV
currently validates **796/796 chunks**, including both chunk sizes and hashes.

## Commands

```text
xet-cdc chunk <file>
xet-cdc validate <file> <reference.chunks>
xet-cdc compare <original> <modified>
```

- `chunk` prints offsets and sizes produced by the GearHash-based CDC algorithm.
- `validate` compares locally computed chunk sizes and hashes with a Hugging Face
  reference manifest.
- `compare` reports chunk and byte reuse between two versions of a file.

## Implemented scope

### Phase 1: exact chunk boundaries

- fixed 256-entry GearHash table;
- 64-bit wrapping arithmetic;
- 8 KiB minimum and 128 KiB maximum chunk sizes, with the final chunk handled
  independently of the minimum;
- rolling-state reset after each emitted boundary;
- incremental input processing so results do not depend on read-buffer size;
- validation of all 796 boundaries in the official reference CSV.

### Phase 2: Xet chunk hashes

- protocol keyed BLAKE3 chunk hashing;
- parsing of the published `<hash> <length>` manifest;
- validation of both chunk length and hash;
- explicit Xet hash-string byte-order conversion.

### Phase 3: deduplication experiment

- deterministic comparison of original and modified files;
- replacement, insertion, and deletion experiments;
- content-addressed reuse based on chunk hashes rather than offsets;
- reporting of reused chunks, new chunks, reused bytes, and reuse percentage.

## Reproducing the Hugging Face reference validation

The reference data is published by Hugging Face in the
[`xet-team/xet-spec-reference-files`](https://huggingface.co/datasets/xet-team/xet-spec-reference-files)
dataset repository.

With the Hugging Face `hf` CLI installed, download the reference CSV and manifest into
`reference-data/`:

```bash
hf download xet-team/xet-spec-reference-files \
  Electric_Vehicle_Population_Data_20250917.csv \
  Electric_Vehicle_Population_Data_20250917.csv.chunks \
  --repo-type dataset \
  --local-dir reference-data
```

The `reference-data/` directory is ignored by Git because the upstream Hugging Face
repository remains the source of truth.

Build and run the validator:

```bash
cmake --preset release
cmake --build --preset release --parallel

./build/release/xet-cdc validate \
  reference-data/Electric_Vehicle_Population_Data_20250917.csv \
  reference-data/Electric_Vehicle_Population_Data_20250917.csv.chunks
```

Expected result:

```text
Validation successful: 796 chunks matched.
```

This checks the full local pipeline:

1. content-defined chunking;
2. keyed BLAKE3 hashing;
3. Xet hash-string conversion;
4. comparison against Hugging Face's published reference manifest.

## Benchmarks

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

The dependency direction is:

```text
CLI -> validation / comparison -> chunk hashing -> CDC boundary detector
```

The core CDC code does not depend on filesystem or command-line concerns. Its state
survives arbitrary input-buffer boundaries, allowing tests to feed identical data using
different block sizes and verify streaming determinism.

Core types and components include:

- `ChunkBoundary`: byte offset and length;
- `GearHash`: the protocol's 64-bit rolling state;
- `Chunker`: consumes byte spans and emits boundaries;
- `ChunkHash`: strongly typed 32-byte chunk hash;
- `HashedChunk`: a boundary paired with its content hash;
- `ReferenceChunk` / reference-manifest parsing: published Xet reference data;
- `ChunkReuseResult`: chunk- and byte-level reuse statistics.

## What this project demonstrates

The project is intentionally small, but it exercises several systems concerns that are
easy to get subtly wrong:

- implementing protocol behavior from a published specification rather than copying an
  existing client implementation;
- keeping CDC output deterministic across arbitrary streaming read boundaries;
- matching an external correctness oracle exactly, including hash representation;
- separating content identity from file offsets when measuring deduplication;
- validating behavior across GCC and Clang, Debug and Release builds, and under
  AddressSanitizer and UndefinedBehaviorSanitizer.

The interesting part is not merely finding *some* chunk boundaries: an independent
implementation must reproduce the exact boundaries and hashes expected by the Xet
reference manifest. Small mistakes in boundary conditions, rolling-state reset, keyed
hashing, or hash-string conversion are enough to break interoperability.

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

The `release` and `asan` presets provide a Release build and a Debug build with
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

## Non-goals

- implementing Xorbs, shards, reconstruction, authentication, or CAS networking;
- competing with the optimized Rust implementation in `xet-core`;
- claiming full Xet protocol compatibility: this project deliberately focuses on CDC,
  chunk hashing, reference validation, and chunk-level reuse behavior.
