#pragma once

#include "xet_cdc/chunk_hash.hpp"
#include "xet_cdc/hashed_chunk.hpp"
#include "xet_cdc/reference_manifest.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <variant>

namespace xet::cdc {

// The first chunk whose computed size differs from the reference manifest.
struct ChunkSizeMismatch {
    std::size_t index{};
    std::uint32_t expected_size{};
    std::uint32_t actual_size{};

    friend constexpr bool operator==(const ChunkSizeMismatch&, const ChunkSizeMismatch&) = default;
};

// The first chunk that has the reference size but not the reference hash.
struct ChunkHashMismatch {
    std::size_t index{};
    ChunkHash expected_hash{};
    ChunkHash actual_hash{};

    friend constexpr bool operator==(const ChunkHashMismatch&, const ChunkHashMismatch&) = default;
};

// Every shared chunk matched, but the two sequences differ in length.
struct ChunkCountMismatch {
    std::size_t expected_count{};
    std::size_t actual_count{};

    friend constexpr bool operator==(const ChunkCountMismatch&,
                                     const ChunkCountMismatch&) = default;
};

using ChunkValidationMismatch =
    std::variant<ChunkSizeMismatch, ChunkHashMismatch, ChunkCountMismatch>;

// Compares computed chunks with the reference manifest entries, in order, and
// returns the first mismatch found.
//
// Each corresponding pair is checked size first and hash second, so a chunk
// that differs in both is reported as a size mismatch: the size explains the
// hash. The chunk count is only compared once every shared chunk matched, which
// keeps the more specific diagnosis in front of the more general one.
//
// Returns std::nullopt when the two sequences agree in count, size and hash.
[[nodiscard]] std::optional<ChunkValidationMismatch>
compare_chunks(std::span<const HashedChunk> actual, std::span<const ReferenceChunk> expected);

} // namespace xet::cdc
