#include "xet_cdc/chunk_validation.hpp"

#include "xet_cdc/protocol.hpp"

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <variant>
#include <vector>

namespace {

using namespace xet::cdc;

[[nodiscard]] std::vector<ChunkBoundary> boundaries_from(const std::vector<std::uint32_t>& sizes) {
    std::vector<ChunkBoundary> boundaries;

    std::uint64_t offset = 0;

    for (const std::uint32_t size : sizes) {
        boundaries.push_back(ChunkBoundary{offset, size});
        offset += size;
    }

    return boundaries;
}

TEST_CASE("compare_chunk_sizes accepts matching sizes", "[chunk_validation]") {
    const std::vector<std::uint32_t> expected{131072, 106099, 61389};

    REQUIRE_FALSE(compare_chunk_sizes(boundaries_from(expected), expected).has_value());
}

TEST_CASE("compare_chunk_sizes accepts two empty sequences", "[chunk_validation]") {
    REQUIRE_FALSE(compare_chunk_sizes({}, {}).has_value());
}

TEST_CASE("compare_chunk_sizes reports the first size mismatch", "[chunk_validation]") {
    const std::vector<std::uint32_t> expected{131072, 62411, 61389};
    const std::vector<std::uint32_t> actual{131072, 65536, 40000};

    const auto mismatch = compare_chunk_sizes(boundaries_from(actual), expected);

    REQUIRE(mismatch.has_value());
    REQUIRE(std::holds_alternative<ChunkSizeMismatch>(*mismatch));
    REQUIRE(std::get<ChunkSizeMismatch>(*mismatch) == ChunkSizeMismatch{1, 62411, 65536});
}

TEST_CASE("compare_chunk_sizes reports a chunk count mismatch", "[chunk_validation]") {
    const std::vector<std::uint32_t> expected{131072, 106099, 61389};
    const std::vector<std::uint32_t> actual{131072, 106099};

    const auto mismatch = compare_chunk_sizes(boundaries_from(actual), expected);

    REQUIRE(mismatch.has_value());
    REQUIRE(std::holds_alternative<ChunkCountMismatch>(*mismatch));
    REQUIRE(std::get<ChunkCountMismatch>(*mismatch) == ChunkCountMismatch{3, 2});
}

TEST_CASE("compare_chunk_sizes prefers a size mismatch over a count mismatch",
          "[chunk_validation]") {
    const std::vector<std::uint32_t> expected{131072, 106099, 61389};
    const std::vector<std::uint32_t> actual{131072, 65536};

    const auto mismatch = compare_chunk_sizes(boundaries_from(actual), expected);

    REQUIRE(mismatch.has_value());
    REQUIRE(std::get<ChunkSizeMismatch>(*mismatch) == ChunkSizeMismatch{1, 106099, 65536});
}

TEST_CASE("compare_chunk_sizes reports extra chunks", "[chunk_validation]") {
    const std::vector<std::uint32_t> expected{131072};
    const std::vector<std::uint32_t> actual{131072, 106099};

    const auto mismatch = compare_chunk_sizes(boundaries_from(actual), expected);

    REQUIRE(mismatch.has_value());
    REQUIRE(std::get<ChunkCountMismatch>(*mismatch) == ChunkCountMismatch{1, 2});
}

} // namespace
