#pragma once

#include "xet_cdc/chunker.hpp"
#include "xet_cdc/hashed_chunk.hpp"

#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace xet::cdc {

// A streaming adapter around Chunker that emits one HashedChunk per CDC chunk:
// the boundary, plus the keyed BLAKE3 hash of exactly the bytes that boundary
// covers.
//
// It exists because the two concerns are not aligned in time. A single
// consume() call may complete several chunks, a chunk may span many calls, and
// a boundary usually falls in the middle of the caller's buffer. Chunker only
// reports boundaries, so something has to map each emitted boundary back onto
// its exact byte range. Doing that here keeps Chunker a pure boundary detector.
//
// Like Chunker, the result depends only on the byte stream, never on how that
// stream is split into spans.
class HashingChunker {
  public:
    HashingChunker() = default;

    // Consumes the next bytes of the stream and returns, in order, every chunk
    // those bytes completed. Returns an empty vector while a chunk is still
    // being accumulated.
    [[nodiscard]] std::vector<HashedChunk> consume(std::span<const std::uint8_t> data);

    // Emits the final chunk, which may be smaller than the protocol minimum.
    // Returns std::nullopt when no bytes are pending, and never emits the same
    // final chunk twice.
    [[nodiscard]] std::optional<HashedChunk> finish();

  private:
    Chunker chunker_;

    // The bytes of the chunk currently being accumulated, that is the bytes
    // consumed since the last emitted boundary. A chunk never exceeds
    // kMaxChunkSize, so buffering it whole and calling hash_chunk() once is
    // enough; no incremental hash state is required.
    std::vector<std::uint8_t> pending_;
};

} // namespace xet::cdc
