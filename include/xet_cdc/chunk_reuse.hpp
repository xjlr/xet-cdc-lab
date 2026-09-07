#pragma once

#include "xet_cdc/hashed_chunk.hpp"

#include <cstddef>
#include <cstdint>
#include <span>

namespace xet::cdc {

// How much of a modified file's chunk content already exists in the original
// file, measured in whole chunks and in bytes.
//
// The counts and the byte totals answer different questions and are both kept:
// CDC chunk sizes vary, so a reused chunk count says nothing about how many
// bytes an upload could skip. Presentation-level figures such as a reuse ratio
// are derived from these fields rather than stored here.
struct ChunkReuseResult {
    std::size_t original_chunk_count{};
    std::size_t modified_chunk_count{};

    // Every modified chunk is either reused or new, so these two always sum to
    // modified_chunk_count. Neither counts anything on the original side.
    std::size_t reused_chunk_count{};
    std::size_t new_chunk_count{};

    std::uint64_t original_bytes{};
    std::uint64_t modified_bytes{};

    // Likewise these two always sum to modified_bytes.
    std::uint64_t reused_bytes{};
    std::uint64_t new_bytes{};

    friend constexpr bool operator==(const ChunkReuseResult&, const ChunkReuseResult&) = default;
};

// Measures how much of `modified` can be reused from `original`.
//
// Reuse is content-addressed: a modified chunk is reusable when a chunk with
// the same ChunkHash appears anywhere in the original. Neither position nor offset takes part.
// After a local edit perturbs the chunking, CDC can resynchronize,
// allowing later unchanged chunks to be reused even though their offsets have shifted.
//
// Matching is by content existence, not by one-to-one pairing: the original's
// multiplicity does not cap reuse, so three copies of a chunk in `modified` are
// all reusable when the original holds one copy. A hash set of the original's
// hashes therefore models the question exactly.
//
// Chunk size never decides reuse. Two chunks of equal size with different
// hashes are different content; the same hash at a different offset is the same
// content.
//
// Every statistic except original_chunk_count and original_bytes describes the
// modified side alone.
[[nodiscard]] ChunkReuseResult compare_chunk_reuse(std::span<const HashedChunk> original,
                                                   std::span<const HashedChunk> modified);

} // namespace xet::cdc
