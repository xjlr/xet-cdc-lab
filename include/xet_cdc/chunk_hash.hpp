#pragma once

#include <array>
#include <cstdint>
#include <span>

namespace xet::cdc {

// The raw 32-byte Xet chunk hash. Conversion to and from the protocol's hash
// string is a separate concern and is not part of this type yet.
struct ChunkHash {
    std::array<std::uint8_t, 32> bytes{};

    friend constexpr bool operator==(const ChunkHash&, const ChunkHash&) = default;
};

// Computes the chunk hash of a complete chunk: keyed BLAKE3 over the chunk
// bytes, using the protocol's fixed data key.
[[nodiscard]] ChunkHash hash_chunk(std::span<const std::uint8_t> data);

} // namespace xet::cdc
