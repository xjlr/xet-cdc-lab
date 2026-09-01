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
- enforce the 8 KiB minimum and 128 KiB maximum chunk sizes;
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

Performance benchmarking is intentionally postponed until correctness is demonstrated.

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
- CMake 3.20+.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/xet-cdc --help
```

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

