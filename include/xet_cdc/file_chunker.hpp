#pragma once

#include "xet_cdc/hashed_chunk.hpp"
#include "xet_cdc/protocol.hpp"

#include <filesystem>
#include <vector>

namespace xet::cdc {

// Reads the file incrementally and returns every chunk boundary it contains,
// including the final one. Printing and other CLI concerns stay out of here.
//
// Throws std::runtime_error if the file cannot be opened or read.
[[nodiscard]] std::vector<ChunkBoundary> chunk_file(const std::filesystem::path& path);

// Reads the file incrementally and returns one HashedChunk per CDC chunk: the
// boundary, plus the keyed BLAKE3 hash of exactly the bytes it covers. The
// boundaries are the ones chunk_file() reports for the same file.
//
// The read buffer size is an internal throughput detail and never affects the
// result.
//
// Throws std::runtime_error if the file cannot be opened or read.
[[nodiscard]] std::vector<HashedChunk> hash_file_chunks(const std::filesystem::path& path);

} // namespace xet::cdc
