#include "xet_cdc/chunk_hash.hpp"

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <string_view>
#include <vector>

namespace {

using xet::cdc::ChunkHash;
using xet::cdc::hash_chunk;

// The hash is exactly 32 raw bytes, with no padding or extra members.
static_assert(sizeof(ChunkHash::bytes) == 32);
static_assert(sizeof(ChunkHash) == 32);

// The expected digests below were produced from the BLAKE3 specification with
// the Xet data key, independently of hash_chunk(). Two implementations agree on
// them: a from-spec port of the official BLAKE3 reference implementation, which
// reproduces all 35 published BLAKE3 test vectors, and the upstream BLAKE3 C
// library called through blake3_hasher_init_keyed().

[[nodiscard]] std::vector<std::uint8_t> bytes_of(std::string_view text) {
    return {text.begin(), text.end()};
}

TEST_CASE("hash_chunk matches the known digest of an empty chunk", "[chunk_hash]") {
    constexpr ChunkHash expected{{
        0x10, 0x5f, 0x7e, 0x4e, 0x78, 0xcf, 0xf2, 0xe0, 0x5f, 0x9a, 0x0e,
        0x15, 0xaf, 0x84, 0x4f, 0xc3, 0x15, 0xd9, 0xba, 0xde, 0x16, 0x42,
        0x66, 0xf9, 0x67, 0x0f, 0x87, 0x49, 0x10, 0x74, 0x4d, 0x36,
    }};

    REQUIRE(hash_chunk({}) == expected);
}

TEST_CASE("hash_chunk matches a known text vector", "[chunk_hash]") {
    constexpr ChunkHash expected{{
        0x0b, 0x05, 0x98, 0xd9, 0x12, 0xba, 0x76, 0x90, 0x3e, 0x94, 0x0f,
        0x77, 0xbf, 0xc8, 0x95, 0x9c, 0xb7, 0x82, 0x0a, 0x49, 0xc7, 0x55,
        0xad, 0x6c, 0xc2, 0xc2, 0x05, 0x2a, 0x2a, 0x58, 0xf1, 0x6c,
    }};

    const std::vector<std::uint8_t> data = bytes_of("hello");

    REQUIRE(hash_chunk(data) == expected);
}

TEST_CASE("hash_chunk matches a known binary vector", "[chunk_hash]") {
    constexpr ChunkHash expected{{
        0x95, 0xc2, 0xfb, 0xa7, 0x8e, 0x22, 0x57, 0x9f, 0x96, 0x3f, 0x0f,
        0xde, 0xce, 0xcd, 0x01, 0x9b, 0xf3, 0x52, 0x23, 0x44, 0xcc, 0xcf,
        0xf0, 0x88, 0x22, 0x02, 0xe2, 0xdf, 0xa9, 0xd4, 0x20, 0xb0,
    }};

    // The 256 bytes 0x00, 0x01, ..., 0xFF, in order.
    std::vector<std::uint8_t> data;
    data.reserve(256);

    for (std::uint16_t byte = 0; byte < 256; ++byte) {
        data.push_back(static_cast<std::uint8_t>(byte));
    }

    REQUIRE(hash_chunk(data) == expected);
}

TEST_CASE("hash_chunk distinguishes different chunks", "[chunk_hash]") {
    REQUIRE(hash_chunk(bytes_of("hello")) != hash_chunk(bytes_of("hellp")));
}

} // namespace
