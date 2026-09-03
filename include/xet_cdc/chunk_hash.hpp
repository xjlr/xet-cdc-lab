#pragma once

#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>

namespace xet::cdc {

// The raw 32-byte Xet chunk hash. It is deliberately distinct from the
// protocol's hash string; to_xet_hex() and parse_xet_hash() below convert
// between the two.
struct ChunkHash {
    std::array<std::uint8_t, 32> bytes{};

    friend constexpr bool operator==(const ChunkHash&, const ChunkHash&) = default;
};

// Computes the chunk hash of a complete chunk: keyed BLAKE3 over the chunk
// bytes, using the protocol's fixed data key.
[[nodiscard]] ChunkHash hash_chunk(std::span<const std::uint8_t> data);

// Encodes a raw hash as the protocol's 64-character hash string, as it appears
// in `.chunks` manifests, shards and CAS API paths.
//
// The encoding is NOT a byte-for-byte hex dump. The raw hash is read as four
// little-endian 64-bit words, so each 8-byte group (indices 0-7, 8-15, 16-23,
// 24-31) is reversed before being written out in lowercase hex:
//
//     raw    61 16 c6 71 dc a1 0a b1  aa 88 a1 41 0c 28 92 de ...
//     string b10aa1dc71c61661         de92280c41a188aa         ...
//
// https://huggingface.co/docs/xet/api#converting-hashes-to-strings
[[nodiscard]] std::string to_xet_hex(const ChunkHash& hash);

// Decodes the protocol's hash string back into the raw hash, inverting
// to_xet_hex().
//
// Only the canonical form is accepted: exactly 64 lowercase hex digits and
// nothing else. This is stricter than the reference implementation, which also
// accepts uppercase, and it keeps to_xet_hex(parse_xet_hash(text)) == text true
// for every string that parses.
//
// Throws std::runtime_error if the text is not a canonical hash string.
[[nodiscard]] ChunkHash parse_xet_hash(std::string_view text);

} // namespace xet::cdc
