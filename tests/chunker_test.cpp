#include "xet_cdc/chunker.hpp"

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstdint>

namespace {

using namespace xet::cdc;

TEST_CASE("Chunker does not emit before minimum chunk size", "[chunker]") {
    Chunker chunker;

    for (std::size_t i = 0; i < kMinChunkSize - 1; ++i) {
        REQUIRE_FALSE(chunker.update(0x42).has_value());
    }
}

TEST_CASE("Chunker emits only valid chunk sizes", "[chunker]") {
    Chunker chunker;

    for (std::size_t i = 0; i < 20 * kMaxChunkSize; ++i) {
        const auto boundary =
            chunker.update(static_cast<std::uint8_t>(i & 0xFF));

        if (!boundary) {
            continue;
        }

        REQUIRE(boundary->size >= kMinChunkSize);
        REQUIRE(boundary->size <= kMaxChunkSize);
    }
}

TEST_CASE("Chunker emits contiguous boundaries", "[chunker]") {
    Chunker chunker;

    std::uint64_t expected_offset = 0;
    std::size_t emitted_boundaries = 0;

    for (std::size_t i = 0; i < 20 * kMaxChunkSize; ++i) {
        const auto boundary =
            chunker.update(static_cast<std::uint8_t>(i & 0xFF));

        if (!boundary) {
            continue;
        }

        REQUIRE(boundary->offset == expected_offset);
        REQUIRE(boundary->end_offset() ==
                expected_offset + boundary->size);

        expected_offset += boundary->size;
        ++emitted_boundaries;
    }

    REQUIRE(emitted_boundaries > 0);
}

TEST_CASE("Two Chunkers produce identical boundaries for identical input",
          "[chunker]") {
    Chunker first;
    Chunker second;

    for (std::size_t i = 0; i < 20 * kMaxChunkSize; ++i) {
        const auto byte = static_cast<std::uint8_t>(i & 0xFF);

        const auto first_boundary = first.update(byte);
        const auto second_boundary = second.update(byte);

        REQUIRE(first_boundary == second_boundary);
    }
}

} // namespace