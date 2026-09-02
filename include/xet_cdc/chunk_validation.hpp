#pragma once

#include "xet_cdc/protocol.hpp"

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

// Every shared chunk matched, but the two sequences differ in length.
struct ChunkCountMismatch {
    std::size_t expected_count{};
    std::size_t actual_count{};

    friend constexpr bool operator==(const ChunkCountMismatch&,
                                     const ChunkCountMismatch&) = default;
};

using ChunkSizeMismatchReport = std::variant<ChunkSizeMismatch, ChunkCountMismatch>;

// Compares computed boundary sizes with the reference sizes, in order. Returns
// std::nullopt when both the sizes and the chunk count match exactly.
[[nodiscard]] std::optional<ChunkSizeMismatchReport>
compare_chunk_sizes(std::span<const ChunkBoundary> actual, std::span<const std::uint32_t> expected);

} // namespace xet::cdc
