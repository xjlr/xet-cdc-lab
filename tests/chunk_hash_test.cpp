#include "xet_cdc/chunk_hash.hpp"

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

using xet::cdc::ChunkHash;
using xet::cdc::hash_chunk;
using xet::cdc::parse_xet_hash;
using xet::cdc::to_xet_hex;

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

// The Xet hash string is not a byte-for-byte hex dump of the raw digest. Each
// 8-byte group of the raw hash (indices 0-7, 8-15, 16-23, 24-31) is read as a
// little-endian 64-bit word, so the group's bytes are reversed before being
// written out in lowercase hex.
// https://huggingface.co/docs/xet/api#converting-hashes-to-strings
//
// The vectors below are fixed constants derived from that rule, never from
// to_xet_hex() or parse_xet_hash().

// The worked example from the specification: the raw bytes 0x00..0x1f, whose
// documented reordering is [7, 6, 5, 4, 3, 2, 1, 0, 15, 14, ..., 24].
constexpr ChunkHash kSpecExampleHash{{
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
}};
constexpr std::string_view kSpecExampleText =
    "07060504030201000f0e0d0c0b0a090817161514131211101f1e1d1c1b1a1918";

// The first chunk of the reference file, as published by Hugging Face in
// Electric_Vehicle_Population_Data_20250917.csv.chunks. The string is real
// protocol output; the raw bytes are its documented reordering.
constexpr ChunkHash kFirstChunkHash{{
    0x61, 0x16, 0xc6, 0x71, 0xdc, 0xa1, 0x0a, 0xb1, 0xaa, 0x88, 0xa1, 0x41, 0x0c, 0x28, 0x92, 0xde,
    0x72, 0x85, 0xb7, 0x39, 0x17, 0x98, 0x47, 0xbc, 0xe4, 0x5c, 0xdc, 0xd8, 0x45, 0x99, 0x09, 0x4a,
}};
constexpr std::string_view kFirstChunkText =
    "b10aa1dc71c61661de92280c41a188aabc47981739b785724a099945d8dc5ce4";

// The second chunk of the same reference manifest.
constexpr ChunkHash kSecondChunkHash{{
    0x6b, 0x3b, 0x80, 0xfa, 0x91, 0x55, 0x25, 0x26, 0x6f, 0x8a, 0x5b, 0x31, 0x8c, 0xd8, 0x25, 0xaf,
    0xc5, 0x8e, 0xf1, 0xfd, 0xbc, 0xd5, 0x53, 0x51, 0x07, 0xc9, 0xcc, 0xe0, 0x64, 0x62, 0x52, 0xef,
}};
constexpr std::string_view kSecondChunkText =
    "26255591fa803b6baf25d88c315b8a6f5153d5bcfdf18ec5ef526264e0ccc907";

TEST_CASE("to_xet_hex encodes the specification's worked example", "[chunk_hash][xet_hex]") {
    REQUIRE(to_xet_hex(kSpecExampleHash) == std::string{kSpecExampleText});
}

TEST_CASE("to_xet_hex encodes a published reference chunk hash", "[chunk_hash][xet_hex]") {
    REQUIRE(to_xet_hex(kFirstChunkHash) == std::string{kFirstChunkText});
    REQUIRE(to_xet_hex(kSecondChunkHash) == std::string{kSecondChunkText});
}

TEST_CASE("to_xet_hex is not a plain hex dump of the raw bytes", "[chunk_hash][xet_hex]") {
    REQUIRE(to_xet_hex(kSpecExampleHash) !=
            "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f");
}

TEST_CASE("parse_xet_hash decodes the specification's worked example", "[chunk_hash][xet_hex]") {
    REQUIRE(parse_xet_hash(kSpecExampleText) == kSpecExampleHash);
}

TEST_CASE("parse_xet_hash decodes a published reference chunk hash", "[chunk_hash][xet_hex]") {
    REQUIRE(parse_xet_hash(kFirstChunkText) == kFirstChunkHash);
    REQUIRE(parse_xet_hash(kSecondChunkText) == kSecondChunkHash);
}

TEST_CASE("parse_xet_hash round-trips a raw hash", "[chunk_hash][xet_hex]") {
    REQUIRE(parse_xet_hash(to_xet_hex(kSpecExampleHash)) == kSpecExampleHash);
    REQUIRE(parse_xet_hash(to_xet_hex(kFirstChunkHash)) == kFirstChunkHash);

    const ChunkHash hello_hash = hash_chunk(bytes_of("hello"));
    REQUIRE(parse_xet_hash(to_xet_hex(hello_hash)) == hello_hash);
}

TEST_CASE("to_xet_hex round-trips a hash string", "[chunk_hash][xet_hex]") {
    REQUIRE(to_xet_hex(parse_xet_hash(kFirstChunkText)) == std::string{kFirstChunkText});

    REQUIRE(to_xet_hex(parse_xet_hash(kSecondChunkText)) == std::string{kSecondChunkText});
}

TEST_CASE("to_xet_hex produces 64 lowercase hex characters", "[chunk_hash][xet_hex]") {
    for (const ChunkHash& hash : {kSpecExampleHash, kFirstChunkHash, kSecondChunkHash}) {
        const std::string text = to_xet_hex(hash);

        REQUIRE(text.size() == 64);
        REQUIRE(text.find_first_not_of("0123456789abcdef") == std::string::npos);
    }
}

TEST_CASE("parse_xet_hash rejects a malformed hash string", "[chunk_hash][xet_hex]") {
    SECTION("empty") { REQUIRE_THROWS_AS(parse_xet_hash(""), std::runtime_error); }

    SECTION("too short") {
        REQUIRE_THROWS_AS(parse_xet_hash(kFirstChunkText.substr(0, 63)), std::runtime_error);
    }

    SECTION("too long") {
        REQUIRE_THROWS_AS(parse_xet_hash(std::string{kFirstChunkText} + "0"), std::runtime_error);
    }

    SECTION("non-hex character") {
        REQUIRE_THROWS_AS(
            parse_xet_hash("g10aa1dc71c61661de92280c41a188aabc47981739b785724a099945d8dc5ce4"),
            std::runtime_error);
    }

    SECTION("embedded space") {
        REQUIRE_THROWS_AS(
            parse_xet_hash("b10aa1dc71c6166 de92280c41a188aabc47981739b785724a099945d8dc5ce4"),
            std::runtime_error);
    }

    SECTION("uppercase is not the canonical form") {
        REQUIRE_THROWS_AS(
            parse_xet_hash("B10AA1DC71C61661DE92280C41A188AABC47981739B785724A099945D8DC5CE4"),
            std::runtime_error);
    }

    SECTION("trailing newline, as read from a manifest line") {
        REQUIRE_THROWS_AS(parse_xet_hash(std::string{kFirstChunkText} + "\n"), std::runtime_error);
    }
}
} // namespace
