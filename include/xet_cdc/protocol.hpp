#pragma once

#include <cstddef>
#include <cstdint>

namespace xet::cdc {

inline constexpr std::size_t kKib = 1024;

// The target size is statistical; it is encoded by the number of set bits in
// kBoundaryMask rather than used directly by the boundary detector.
inline constexpr std::size_t kTargetChunkSize = 64 * kKib;

// The final chunk may be smaller than this value. Files smaller than the
// minimum are emitted whole as a single final chunk.
inline constexpr std::size_t kMinChunkSize = 8 * kKib;

// A boundary is forced when the current chunk reaches this size.
inline constexpr std::size_t kMaxChunkSize = 128 * kKib;

inline constexpr std::uint64_t kBoundaryMask = 0xFFFF'0000'0000'0000ULL;

struct ChunkBoundary {
    std::uint64_t offset{};
    std::uint32_t size{};

    [[nodiscard]] constexpr std::uint64_t end_offset() const noexcept { return offset + size; }

    friend constexpr bool operator==(const ChunkBoundary&, const ChunkBoundary&) = default;
};

} // namespace xet::cdc
