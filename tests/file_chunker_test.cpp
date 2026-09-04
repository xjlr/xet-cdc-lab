#include "xet_cdc/file_chunker.hpp"

#include "xet_cdc/chunk_hash.hpp"
#include "xet_cdc/chunker.hpp"
#include "xet_cdc/hashed_chunk.hpp"
#include "xet_cdc/hashing_chunker.hpp"

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <ios>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using namespace xet::cdc;

// Writes a temporary file for a single test case and removes it afterwards.
class TemporaryFile {
  public:
    explicit TemporaryFile(const std::vector<std::uint8_t>& data, const std::string& name)
        : path_(std::filesystem::temp_directory_path() / ("xet_cdc_test_" + name)) {
        std::ofstream output(path_, std::ios::binary | std::ios::trunc);
        output.write(reinterpret_cast<const char*>(data.data()),
                     static_cast<std::streamsize>(data.size()));
    }

    TemporaryFile(const TemporaryFile&) = delete;
    TemporaryFile& operator=(const TemporaryFile&) = delete;

    ~TemporaryFile() {
        std::error_code error;
        std::filesystem::remove(path_, error);
    }

    [[nodiscard]] const std::filesystem::path& path() const noexcept { return path_; }

  private:
    std::filesystem::path path_;
};

[[nodiscard]] std::vector<std::uint8_t> deterministic_data(std::size_t size) {
    std::vector<std::uint8_t> data(size);

    for (std::size_t i = 0; i < data.size(); ++i) {
        data[i] = static_cast<std::uint8_t>((i * 31 + (i >> 8)) & 0xFF);
    }

    return data;
}

[[nodiscard]] std::vector<ChunkBoundary> chunk_in_memory(const std::vector<std::uint8_t>& data) {
    Chunker chunker;

    std::vector<ChunkBoundary> boundaries = chunker.consume(data);

    if (const auto final_boundary = chunker.finish()) {
        boundaries.push_back(*final_boundary);
    }

    return boundaries;
}

[[nodiscard]] std::vector<HashedChunk> hash_in_memory(const std::vector<std::uint8_t>& data) {
    HashingChunker chunker;

    std::vector<HashedChunk> chunks = chunker.consume(data);

    if (const auto final_chunk = chunker.finish()) {
        chunks.push_back(*final_chunk);
    }

    return chunks;
}

TEST_CASE("chunk_file matches in-memory chunking of the same data", "[file_chunker]") {
    const auto data = deterministic_data(5 * kMaxChunkSize + 12345);

    const TemporaryFile file(data, "chunk_file_matches_memory.bin");

    const auto boundaries = chunk_file(file.path());

    REQUIRE(boundaries.size() > 1);
    REQUIRE(boundaries == chunk_in_memory(data));
}

TEST_CASE("chunk_file covers the whole file contiguously", "[file_chunker]") {
    const std::size_t size = 5 * kMaxChunkSize + 12345;

    const TemporaryFile file(deterministic_data(size), "chunk_file_contiguous.bin");

    const auto boundaries = chunk_file(file.path());

    REQUIRE_FALSE(boundaries.empty());

    std::uint64_t expected_offset = 0;

    for (const auto& boundary : boundaries) {
        REQUIRE(boundary.offset == expected_offset);
        expected_offset += boundary.size;
    }

    REQUIRE(expected_offset == size);
}

TEST_CASE("chunk_file emits a single chunk for a small file", "[file_chunker]") {
    constexpr std::size_t size = 100;

    const TemporaryFile file(deterministic_data(size), "chunk_file_small.bin");

    const auto boundaries = chunk_file(file.path());

    REQUIRE(boundaries.size() == 1);
    REQUIRE(boundaries.front().offset == 0);
    REQUIRE(boundaries.front().size == size);
}

TEST_CASE("chunk_file returns no boundary for an empty file", "[file_chunker]") {
    const TemporaryFile file({}, "chunk_file_empty.bin");

    REQUIRE(chunk_file(file.path()).empty());
}

TEST_CASE("chunk_file throws when the file cannot be opened", "[file_chunker]") {
    const auto missing = std::filesystem::temp_directory_path() / "xet_cdc_test_missing.bin";

    std::error_code error;
    std::filesystem::remove(missing, error);

    REQUIRE_THROWS_AS(chunk_file(missing), std::runtime_error);
}

TEST_CASE("hash_file_chunks matches in-memory hashed chunking of the same data", "[file_chunker]") {
    const auto data = deterministic_data(5 * kMaxChunkSize + 12345);

    const TemporaryFile file(data, "hash_file_chunks_matches_memory.bin");

    const auto chunks = hash_file_chunks(file.path());

    REQUIRE(chunks.size() > 1);
    REQUIRE(chunks == hash_in_memory(data));
}

TEST_CASE("hash_file_chunks reports the same boundaries as chunk_file", "[file_chunker]") {
    const auto data = deterministic_data(5 * kMaxChunkSize + 12345);

    const TemporaryFile file(data, "hash_file_chunks_boundaries.bin");

    const auto boundaries = chunk_file(file.path());
    const auto chunks = hash_file_chunks(file.path());

    REQUIRE(chunks.size() == boundaries.size());

    for (std::size_t i = 0; i < chunks.size(); ++i) {
        INFO("chunk index: " << i);
        REQUIRE(chunks[i].boundary == boundaries[i]);
    }
}

TEST_CASE("hash_file_chunks hashes each chunk over exactly its own file bytes", "[file_chunker]") {
    const auto data = deterministic_data(2 * kMaxChunkSize + 999);

    const TemporaryFile file(data, "hash_file_chunks_exact_bytes.bin");

    const auto chunks = hash_file_chunks(file.path());

    REQUIRE(chunks.size() > 1);

    const std::span<const std::uint8_t> bytes(data);

    for (std::size_t i = 0; i < chunks.size(); ++i) {
        INFO("chunk index: " << i);

        const ChunkBoundary& boundary = chunks[i].boundary;

        REQUIRE(chunks[i].hash == hash_chunk(bytes.subspan(
                                      static_cast<std::size_t>(boundary.offset), boundary.size)));
        REQUIRE(chunks[i].hash != hash_chunk(bytes));
    }
}

TEST_CASE("hash_file_chunks emits a single chunk for a small file", "[file_chunker]") {
    constexpr std::size_t size = 100;

    const auto data = deterministic_data(size);

    const TemporaryFile file(data, "hash_file_chunks_small.bin");

    const auto chunks = hash_file_chunks(file.path());

    REQUIRE(chunks.size() == 1);
    REQUIRE(chunks.front().boundary == ChunkBoundary{0, size});
    REQUIRE(chunks.front().hash == hash_chunk(data));
}

TEST_CASE("hash_file_chunks returns no chunk for an empty file", "[file_chunker]") {
    const TemporaryFile file({}, "hash_file_chunks_empty.bin");

    REQUIRE(hash_file_chunks(file.path()).empty());
}

TEST_CASE("hash_file_chunks throws when the file cannot be opened", "[file_chunker]") {
    const auto missing = std::filesystem::temp_directory_path() / "xet_cdc_test_hash_missing.bin";

    std::error_code error;
    std::filesystem::remove(missing, error);

    REQUIRE_THROWS_AS(hash_file_chunks(missing), std::runtime_error);
}

} // namespace
