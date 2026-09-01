#pragma once

#include <cstddef>
#include <cstdint>

namespace xet::cdc {

inline constexpr std::size_t kKib = 1024;
inline constexpr std::size_t kTargetChunkSize = 64 * kKib;
inline constexpr std::size_t kMinChunkSize = 8 * kKib;
inline constexpr std::size_t kMaxChunkSize = 128 * kKib;
inline constexpr std::uint64_t kBoundaryMask = 0xFFFF'0000'0000'0000ULL;

struct ChunkBoundary {
    std::uint64_t offset{};
    std::uint32_t size{};

    [[nodiscard]] constexpr std::uint64_t end_offset() const noexcept {
        return offset + size;
    }

    friend constexpr bool operator==(const ChunkBoundary&, const ChunkBoundary&) = default;
};

} // namespace xet::cdc

