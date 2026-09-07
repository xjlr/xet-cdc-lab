#include "xet_cdc/chunk_validation.hpp"

#include "xet_cdc/chunk_hash.hpp"
#include "xet_cdc/hashed_chunk.hpp"
#include "xet_cdc/protocol.hpp"
#include "xet_cdc/reference_manifest.hpp"

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <variant>
#include <vector>

namespace {

using namespace xet::cdc;

// One chunk of a test sequence: its length, plus a seed standing in for its
// content. Two entries hash equal exactly when their seeds are equal.
struct ChunkSpec {
    std::uint32_t size{};
    std::uint8_t seed{};
};

[[nodiscard]] ChunkHash hash_of(std::uint8_t seed) {
    const std::array<std::uint8_t, 1> content{seed};

    return hash_chunk(content);
}

[[nodiscard]] std::vector<HashedChunk> actual_chunks(const std::vector<ChunkSpec>& specs) {
    std::vector<HashedChunk> chunks;

    std::uint64_t offset = 0;

    for (const ChunkSpec& spec : specs) {
        chunks.push_back(HashedChunk{ChunkBoundary{offset, spec.size}, hash_of(spec.seed)});
        offset += spec.size;
    }

    return chunks;
}

[[nodiscard]] std::vector<ReferenceChunk> reference_chunks(const std::vector<ChunkSpec>& specs) {
    std::vector<ReferenceChunk> chunks;

    for (const ChunkSpec& spec : specs) {
        chunks.push_back(ReferenceChunk{hash_of(spec.seed), spec.size});
    }

    return chunks;
}

TEST_CASE("compare_chunks accepts matching sizes and hashes", "[chunk_validation]") {
    const std::vector<ChunkSpec> specs{{131072, 1}, {106099, 2}, {61389, 3}};

    REQUIRE_FALSE(compare_chunks(actual_chunks(specs), reference_chunks(specs)).has_value());
}

TEST_CASE("compare_chunks accepts two empty sequences", "[chunk_validation]") {
    REQUIRE_FALSE(compare_chunks({}, {}).has_value());
}

TEST_CASE("compare_chunks reports the first size mismatch", "[chunk_validation]") {
    const auto expected = reference_chunks({{131072, 1}, {62411, 2}, {61389, 3}});
    const auto actual = actual_chunks({{131072, 1}, {65536, 2}, {40000, 3}});

    const auto mismatch = compare_chunks(actual, expected);

    REQUIRE(mismatch.has_value());
    REQUIRE(std::holds_alternative<ChunkSizeMismatch>(*mismatch));
    REQUIRE(std::get<ChunkSizeMismatch>(*mismatch) == ChunkSizeMismatch{1, 62411, 65536});
}

TEST_CASE("compare_chunks reports a hash mismatch for a correctly sized chunk",
          "[chunk_validation]") {
    const auto expected = reference_chunks({{131072, 1}, {106099, 2}, {61389, 3}});
    const auto actual = actual_chunks({{131072, 1}, {106099, 9}, {61389, 3}});

    const auto mismatch = compare_chunks(actual, expected);

    REQUIRE(mismatch.has_value());
    REQUIRE(std::holds_alternative<ChunkHashMismatch>(*mismatch));
    REQUIRE(std::get<ChunkHashMismatch>(*mismatch) == ChunkHashMismatch{1, hash_of(2), hash_of(9)});
}

TEST_CASE("compare_chunks prefers a size mismatch over a hash mismatch in the same chunk",
          "[chunk_validation]") {
    const auto expected = reference_chunks({{131072, 1}, {106099, 2}});
    const auto actual = actual_chunks({{131072, 1}, {65536, 9}});

    const auto mismatch = compare_chunks(actual, expected);

    REQUIRE(mismatch.has_value());
    REQUIRE(std::holds_alternative<ChunkSizeMismatch>(*mismatch));
    REQUIRE(std::get<ChunkSizeMismatch>(*mismatch) == ChunkSizeMismatch{1, 106099, 65536});
}

TEST_CASE("compare_chunks reports the first mismatching chunk of several", "[chunk_validation]") {
    const auto expected = reference_chunks({{131072, 1}, {106099, 2}, {61389, 3}, {40000, 4}});
    const auto actual = actual_chunks({{131072, 1}, {106099, 9}, {50000, 3}, {40000, 8}});

    const auto mismatch = compare_chunks(actual, expected);

    REQUIRE(mismatch.has_value());
    REQUIRE(std::get<ChunkHashMismatch>(*mismatch) == ChunkHashMismatch{1, hash_of(2), hash_of(9)});
}

TEST_CASE("compare_chunks reports missing chunks as a count mismatch", "[chunk_validation]") {
    const auto expected = reference_chunks({{131072, 1}, {106099, 2}, {61389, 3}});
    const auto actual = actual_chunks({{131072, 1}, {106099, 2}});

    const auto mismatch = compare_chunks(actual, expected);

    REQUIRE(mismatch.has_value());
    REQUIRE(std::holds_alternative<ChunkCountMismatch>(*mismatch));
    REQUIRE(std::get<ChunkCountMismatch>(*mismatch) == ChunkCountMismatch{3, 2});
}

TEST_CASE("compare_chunks reports extra chunks as a count mismatch", "[chunk_validation]") {
    const auto expected = reference_chunks({{131072, 1}});
    const auto actual = actual_chunks({{131072, 1}, {106099, 2}});

    const auto mismatch = compare_chunks(actual, expected);

    REQUIRE(mismatch.has_value());
    REQUIRE(std::holds_alternative<ChunkCountMismatch>(*mismatch));
    REQUIRE(std::get<ChunkCountMismatch>(*mismatch) == ChunkCountMismatch{1, 2});
}

TEST_CASE("compare_chunks prefers a size mismatch over a count mismatch", "[chunk_validation]") {
    const auto expected = reference_chunks({{131072, 1}, {106099, 2}, {61389, 3}});
    const auto actual = actual_chunks({{131072, 1}, {65536, 2}});

    const auto mismatch = compare_chunks(actual, expected);

    REQUIRE(mismatch.has_value());
    REQUIRE(std::get<ChunkSizeMismatch>(*mismatch) == ChunkSizeMismatch{1, 106099, 65536});
}

TEST_CASE("compare_chunks prefers a hash mismatch over a count mismatch", "[chunk_validation]") {
    const auto expected = reference_chunks({{131072, 1}, {106099, 2}, {61389, 3}});
    const auto actual = actual_chunks({{131072, 1}, {106099, 9}});

    const auto mismatch = compare_chunks(actual, expected);

    REQUIRE(mismatch.has_value());
    REQUIRE(std::get<ChunkHashMismatch>(*mismatch) == ChunkHashMismatch{1, hash_of(2), hash_of(9)});
}

TEST_CASE("compare_chunks ignores chunk offsets", "[chunk_validation]") {
    const std::vector<ChunkSpec> specs{{131072, 1}, {106099, 2}};

    std::vector<HashedChunk> actual = actual_chunks(specs);

    // Only sizes and hashes take part in the comparison; the reference manifest
    // carries no offsets to compare against.
    actual.front().boundary.offset = 1234;

    REQUIRE_FALSE(compare_chunks(actual, reference_chunks(specs)).has_value());
}

} // namespace
