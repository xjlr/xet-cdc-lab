#include "xet_cdc/hashing_chunker.hpp"

#include "xet_cdc/chunk_hash.hpp"
#include "xet_cdc/chunker.hpp"
#include "xet_cdc/hashed_chunk.hpp"
#include "xet_cdc/protocol.hpp"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace {

using namespace xet::cdc;

// A deterministic pseudo-random byte stream. Unlike constant or counter data it
// triggers genuine content-defined boundaries, so the chunks vary in size
// instead of every one of them hitting the forced maximum.
[[nodiscard]] std::vector<std::uint8_t> pseudo_random_data(std::size_t size) {
    std::vector<std::uint8_t> data(size);

    std::uint64_t state = 0x0123456789abcdefULL;

    for (std::size_t i = 0; i < data.size(); ++i) {
        state = state * 6364136223846793005ULL + 1442695040888963407ULL;
        data[i] = static_cast<std::uint8_t>(state >> 33);
    }

    return data;
}

[[nodiscard]] std::span<const std::uint8_t> chunk_bytes(std::span<const std::uint8_t> data,
                                                        const ChunkBoundary& boundary) {
    return data.subspan(static_cast<std::size_t>(boundary.offset), boundary.size);
}

// The boundaries of the whole input, taken straight from the CDC layer.
[[nodiscard]] std::vector<ChunkBoundary> boundaries_of(std::span<const std::uint8_t> data) {
    Chunker chunker;

    std::vector<ChunkBoundary> boundaries = chunker.consume(data);

    if (const auto final_boundary = chunker.finish()) {
        boundaries.push_back(*final_boundary);
    }

    return boundaries;
}

// The expected result, built independently of HashingChunker: the CDC
// boundaries, each paired with hash_chunk() over exactly the bytes that
// boundary covers.
[[nodiscard]] std::vector<HashedChunk> expected_chunks(std::span<const std::uint8_t> data) {
    std::vector<HashedChunk> chunks;

    for (const ChunkBoundary& boundary : boundaries_of(data)) {
        chunks.push_back(HashedChunk{boundary, hash_chunk(chunk_bytes(data, boundary))});
    }

    return chunks;
}

// Feeds the whole stream through one HashingChunker in fixed-size pieces.
[[nodiscard]] std::vector<HashedChunk> hash_in_pieces(std::span<const std::uint8_t> data,
                                                      std::size_t piece_size) {
    HashingChunker chunker;

    std::vector<HashedChunk> chunks;

    for (std::size_t offset = 0; offset < data.size(); offset += piece_size) {
        const std::size_t size = std::min(piece_size, data.size() - offset);

        const auto emitted = chunker.consume(data.subspan(offset, size));

        chunks.insert(chunks.end(), emitted.begin(), emitted.end());
    }

    if (const auto final_chunk = chunker.finish()) {
        chunks.push_back(*final_chunk);
    }

    return chunks;
}

TEST_CASE("HashingChunker emits no chunk for empty input", "[hashing_chunker]") {
    HashingChunker chunker;

    REQUIRE(chunker.consume({}).empty());
    REQUIRE_FALSE(chunker.finish().has_value());
}

TEST_CASE("HashingChunker hashes an input below the minimum chunk size as one final chunk",
          "[hashing_chunker]") {
    constexpr std::size_t input_size = 100;

    const auto data = pseudo_random_data(input_size);

    HashingChunker chunker;

    REQUIRE(chunker.consume(data).empty());

    const auto final_chunk = chunker.finish();

    REQUIRE(final_chunk.has_value());
    REQUIRE(final_chunk->boundary.offset == 0);
    REQUIRE(final_chunk->boundary.size == input_size);
    REQUIRE(final_chunk->hash == hash_chunk(data));

    REQUIRE_FALSE(chunker.finish().has_value());
}

TEST_CASE("HashingChunker hashes a forced maximum-size chunk over exactly its bytes",
          "[hashing_chunker]") {
    // Constant data never satisfies the boundary mask, so the first chunk is
    // cut by the maximum chunk size rather than by the content.
    const std::vector<std::uint8_t> data(kMaxChunkSize + 5000, 0x42);

    const auto chunks = hash_in_pieces(data, data.size());

    REQUIRE(chunks.size() == 2);
    REQUIRE(chunks.front().boundary == ChunkBoundary{0, static_cast<std::uint32_t>(kMaxChunkSize)});
    REQUIRE(chunks.front().hash ==
            hash_chunk(std::span<const std::uint8_t>(data).first(kMaxChunkSize)));

    // Not the whole input, and not the whole input minus nothing: exactly the
    // first kMaxChunkSize bytes.
    REQUIRE(chunks.front().hash != hash_chunk(data));
}

TEST_CASE("HashingChunker hashes every chunk over exactly its own bytes", "[hashing_chunker]") {
    const auto data = pseudo_random_data(1024 * kKib);

    const auto chunks = hash_in_pieces(data, data.size());

    const auto expected = expected_chunks(data);

    REQUIRE(expected.size() > 3);
    REQUIRE(chunks == expected);
}

TEST_CASE("HashingChunker does not hash the whole input for every chunk", "[hashing_chunker]") {
    const auto data = pseudo_random_data(512 * kKib);

    const auto chunks = hash_in_pieces(data, data.size());

    REQUIRE(chunks.size() > 3);

    const ChunkHash whole_input_hash = hash_chunk(data);

    for (const HashedChunk& chunk : chunks) {
        REQUIRE(chunk.hash != whole_input_hash);
    }

    // Distinct byte ranges must produce distinct hashes; equal hashes would
    // mean the same bytes were hashed more than once.
    for (std::size_t i = 1; i < chunks.size(); ++i) {
        REQUIRE(chunks[i].hash != chunks[i - 1].hash);
    }
}

TEST_CASE("HashingChunker hashes the final tail chunk over only the tail", "[hashing_chunker]") {
    // Sized so the stream holds one content-defined boundary followed by a
    // shorter tail that finish() has to emit.
    const auto data = pseudo_random_data(50000);

    const auto boundaries = boundaries_of(data);

    REQUIRE(boundaries.size() == 2);
    REQUIRE(boundaries.back().size < boundaries.front().size);

    const auto chunks = hash_in_pieces(data, data.size());

    REQUIRE(chunks.size() == 2);

    const HashedChunk& tail = chunks.back();

    REQUIRE(tail.boundary == boundaries.back());
    REQUIRE(tail.boundary.offset == boundaries.front().size);
    REQUIRE(tail.boundary.end_offset() == data.size());
    REQUIRE(tail.hash == hash_chunk(chunk_bytes(data, boundaries.back())));

    // The tail is not the whole stream, and not the preceding chunk either.
    REQUIRE(tail.hash != hash_chunk(data));
    REQUIRE(tail.hash != chunks.front().hash);
}

TEST_CASE("HashingChunker results do not depend on how the input is split", "[hashing_chunker]") {
    const auto data = pseudo_random_data(300 * kKib);

    const auto expected = expected_chunks(data);

    REQUIRE(expected.size() > 3);

    const std::size_t piece_sizes[] = {
        data.size(), 1, 17, 4096, 64 * kKib, kMaxChunkSize + 1,
    };

    for (const std::size_t piece_size : piece_sizes) {
        INFO("piece size: " << piece_size);
        REQUIRE(hash_in_pieces(data, piece_size) == expected);
    }
}

TEST_CASE("HashingChunker splits hashes at the CDC boundary, not at the span boundary",
          "[hashing_chunker]") {
    const auto data = pseudo_random_data(300 * kKib);

    const auto boundaries = boundaries_of(data);

    REQUIRE(boundaries.size() > 2);

    // One span that holds the bytes before the first boundary, the boundary
    // itself, and the first bytes of the following chunk.
    const std::size_t first_chunk_size = boundaries.front().size;
    const std::size_t span_size = first_chunk_size + 1000;

    REQUIRE(span_size < first_chunk_size + boundaries[1].size);

    HashingChunker chunker;

    const auto emitted = chunker.consume(std::span<const std::uint8_t>(data).first(span_size));

    REQUIRE(emitted.size() == 1);
    REQUIRE(emitted.front().boundary == boundaries.front());

    // The hash covers exactly the chunk: not the span it arrived in, and not
    // the whole stream.
    REQUIRE(emitted.front().hash == hash_chunk(chunk_bytes(data, boundaries.front())));
    REQUIRE(emitted.front().hash !=
            hash_chunk(std::span<const std::uint8_t>(data).first(span_size)));
    REQUIRE(emitted.front().hash != hash_chunk(data));

    // The bytes after the boundary are kept, not dropped: feeding the rest of
    // the stream must reproduce the full expected result.
    std::vector<HashedChunk> chunks = emitted;

    const auto rest = chunker.consume(std::span<const std::uint8_t>(data).subspan(span_size));

    chunks.insert(chunks.end(), rest.begin(), rest.end());

    if (const auto final_chunk = chunker.finish()) {
        chunks.push_back(*final_chunk);
    }

    REQUIRE(chunks == expected_chunks(data));
}

TEST_CASE("HashingChunker emits several chunks from a single span", "[hashing_chunker]") {
    const auto data = pseudo_random_data(300 * kKib);

    const auto expected = expected_chunks(data);

    HashingChunker chunker;

    const auto emitted = chunker.consume(data);

    REQUIRE(emitted.size() > 2);
    REQUIRE(emitted.size() == expected.size() - 1);

    for (std::size_t i = 0; i < emitted.size(); ++i) {
        INFO("chunk index: " << i);
        REQUIRE(emitted[i] == expected[i]);
    }
}

} // namespace
