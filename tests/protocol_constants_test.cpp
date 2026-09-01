#include "xet_cdc/protocol.hpp"

#include <bit>

#include <catch2/catch_test_macros.hpp>

namespace {

using namespace xet::cdc;

static_assert(kMinChunkSize == 8 * 1024);
static_assert(kTargetChunkSize == 64 * 1024);
static_assert(kMaxChunkSize == 128 * 1024);
static_assert(kBoundaryMask == 0xFFFF'0000'0000'0000ULL);

// With uniformly distributed hash bits, a 16-bit mask gives an expected
// boundary interval of 2^16 bytes, matching the 64 KiB target size.
static_assert(1ULL << std::popcount(kBoundaryMask) == kTargetChunkSize);

static_assert(kMinChunkSize < kTargetChunkSize);
static_assert(kTargetChunkSize < kMaxChunkSize);

} // namespace

TEST_CASE("ChunkBoundary computes its end offset", "[protocol]") {
    constexpr ChunkBoundary boundary{.offset = 100, .size = 25};
    STATIC_REQUIRE(boundary.end_offset() == 125);
}

TEST_CASE("ChunkBoundary uses a 64-bit offset", "[protocol]") {
    constexpr ChunkBoundary boundary{.offset = 0xFFFF'FFFFULL, .size = kMaxChunkSize};
    STATIC_REQUIRE(boundary.end_offset() == 0xFFFF'FFFFULL + kMaxChunkSize);
}

TEST_CASE("ChunkBoundary compares by value", "[protocol]") {
    constexpr ChunkBoundary a{.offset = 8192, .size = 65536};
    constexpr ChunkBoundary b{.offset = 8192, .size = 65536};
    constexpr ChunkBoundary c{.offset = 8192, .size = 65537};

    REQUIRE(a == b);
    REQUIRE(a != c);
}
