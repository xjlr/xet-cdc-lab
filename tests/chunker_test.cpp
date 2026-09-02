#include "xet_cdc/chunker.hpp"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace {

using namespace xet::cdc;

TEST_CASE("Chunker does not emit before minimum chunk size", "[chunker]") {
    Chunker chunker;

    std::vector<std::uint8_t> data(kMinChunkSize - 1, 0x42);

    const auto boundaries = chunker.consume(data);

    REQUIRE(boundaries.empty());
}

TEST_CASE("Chunker emits only valid chunk sizes", "[chunker]") {
    Chunker chunker;

    std::vector<std::uint8_t> data(20 * kMaxChunkSize);

    for (std::size_t i = 0; i < data.size(); ++i) {
        data[i] = static_cast<std::uint8_t>(i & 0xFF);
    }

    const auto boundaries = chunker.consume(data);

    REQUIRE_FALSE(boundaries.empty());

    for (const auto& boundary : boundaries) {
        REQUIRE(boundary.size >= kMinChunkSize);
        REQUIRE(boundary.size <= kMaxChunkSize);
    }
}

TEST_CASE("Chunker emits contiguous boundaries", "[chunker]") {
    Chunker chunker;

    std::vector<std::uint8_t> data(20 * kMaxChunkSize);

    for (std::size_t i = 0; i < data.size(); ++i) {
        data[i] = static_cast<std::uint8_t>(i & 0xFF);
    }

    const auto boundaries = chunker.consume(data);

    REQUIRE_FALSE(boundaries.empty());

    std::uint64_t expected_offset = 0;

    for (const auto& boundary : boundaries) {
        REQUIRE(boundary.offset == expected_offset);
        REQUIRE(boundary.end_offset() == expected_offset + boundary.size);

        expected_offset += boundary.size;
    }
}

TEST_CASE("Two Chunkers produce identical boundaries for identical input", "[chunker]") {
    Chunker first;
    Chunker second;

    std::vector<std::uint8_t> data(20 * kMaxChunkSize);

    for (std::size_t i = 0; i < data.size(); ++i) {
        data[i] = static_cast<std::uint8_t>(i & 0xFF);
    }

    const auto first_boundaries = first.consume(data);
    const auto second_boundaries = second.consume(data);

    REQUIRE(first_boundaries == second_boundaries);
}

TEST_CASE("Chunker produces identical boundaries across different input splits", "[chunker]") {
    std::vector<std::uint8_t> data(20 * kMaxChunkSize);

    for (std::size_t i = 0; i < data.size(); ++i) {
        data[i] = static_cast<std::uint8_t>(i & 0xFF);
    }

    Chunker whole_input_chunker;
    const auto whole_boundaries = whole_input_chunker.consume(data);

    Chunker split_input_chunker;
    std::vector<ChunkBoundary> split_boundaries;

    constexpr std::size_t block_size = 4096;

    for (std::size_t offset = 0; offset < data.size(); offset += block_size) {
        const std::size_t size = std::min(block_size, data.size() - offset);

        const std::span<const std::uint8_t> block(data.data() + offset, size);

        const auto boundaries = split_input_chunker.consume(block);

        split_boundaries.insert(split_boundaries.end(), boundaries.begin(), boundaries.end());
    }

    REQUIRE(split_boundaries == whole_boundaries);
}

TEST_CASE("Chunker finish returns no boundary for empty input", "[chunker]") {
    Chunker chunker;

    const auto boundary = chunker.finish();

    REQUIRE_FALSE(boundary.has_value());
}

TEST_CASE("Chunker finish emits a final chunk smaller than minimum size", "[chunker]") {
    Chunker chunker;

    constexpr std::size_t input_size = 100;

    std::vector<std::uint8_t> data(input_size, 0x42);

    const auto boundaries = chunker.consume(data);

    REQUIRE(boundaries.empty());

    const auto boundary = chunker.finish();

    REQUIRE(boundary.has_value());
    REQUIRE(boundary->offset == 0);
    REQUIRE(boundary->size == input_size);
}

TEST_CASE("Chunker finish emits the remaining tail with correct offset", "[chunker]") {
    Chunker chunker;

    std::optional<ChunkBoundary> emitted_boundary;

    for (std::size_t i = 0; i < 10 * kMaxChunkSize && !emitted_boundary; ++i) {
        const std::uint8_t byte = static_cast<std::uint8_t>(i & 0xFF);
        const std::span<const std::uint8_t> input(&byte, 1);

        const auto boundaries = chunker.consume(input);

        if (!boundaries.empty()) {
            REQUIRE(boundaries.size() == 1);
            emitted_boundary = boundaries.front();
        }
    }

    REQUIRE(emitted_boundary.has_value());

    constexpr std::size_t tail_size = 100;
    std::vector<std::uint8_t> tail(tail_size, 0x42);

    const auto tail_boundaries = chunker.consume(tail);

    REQUIRE(tail_boundaries.empty());

    const auto final_boundary = chunker.finish();

    REQUIRE(final_boundary.has_value());
    REQUIRE(final_boundary->offset == emitted_boundary->end_offset());
    REQUIRE(final_boundary->size == tail_size);
}

TEST_CASE("Chunker finish does not emit the final chunk twice", "[chunker]") {
    Chunker chunker;

    constexpr std::size_t input_size = 100;

    std::vector<std::uint8_t> data(input_size, 0x42);

    const auto boundaries = chunker.consume(data);

    REQUIRE(boundaries.empty());

    const auto first = chunker.finish();
    const auto second = chunker.finish();

    REQUIRE(first.has_value());
    REQUIRE(first->offset == 0);
    REQUIRE(first->size == input_size);

    REQUIRE_FALSE(second.has_value());
}

} // namespace