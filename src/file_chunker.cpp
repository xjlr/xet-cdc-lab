#include "xet_cdc/file_chunker.hpp"

#include "xet_cdc/chunker.hpp"
#include "xet_cdc/hashing_chunker.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <ios>
#include <span>
#include <stdexcept>

namespace xet::cdc {
namespace {

// The chunker is streaming, so this size affects throughput only, never results.
constexpr std::size_t kReadBufferSize = 64 * kKib;

} // namespace

std::vector<ChunkBoundary> chunk_file(const std::filesystem::path& path) {
    std::ifstream input_file(path, std::ios::binary);
    if (!input_file) {
        throw std::runtime_error("Error opening file: " + path.string());
    }

    Chunker chunker;
    std::vector<ChunkBoundary> boundaries;

    std::array<std::uint8_t, kReadBufferSize> buffer{};

    while (input_file) {
        input_file.read(reinterpret_cast<char*>(buffer.data()),
                        static_cast<std::streamsize>(buffer.size()));

        const std::streamsize bytes_read = input_file.gcount();

        if (bytes_read > 0) {
            const auto data =
                std::span<const std::uint8_t>(buffer.data(), static_cast<std::size_t>(bytes_read));

            const auto emitted = chunker.consume(data);

            boundaries.insert(boundaries.end(), emitted.begin(), emitted.end());
        }
    }

    if (input_file.bad()) {
        throw std::runtime_error("Error reading file: " + path.string());
    }

    if (const auto final_boundary = chunker.finish()) {
        boundaries.push_back(*final_boundary);
    }

    return boundaries;
}

std::vector<HashedChunk> hash_file_chunks(const std::filesystem::path& path) {
    // TODO: stream the file exactly as chunk_file() does above -- same open and
    // read-error handling, same kReadBufferSize reads -- but feed the buffers
    // to a HashingChunker instead of a Chunker, appending every emitted
    // HashedChunk and finally the one HashingChunker::finish() returns.
    std::ifstream input_file(path, std::ios::binary);
    if (!input_file) {
        throw std::runtime_error("Error opening file: " + path.string());
    }
    HashingChunker hashing_chunker;
    std::vector<HashedChunk> hashed_chunks;
    std::array<std::uint8_t, kReadBufferSize> buffer{};
    while (input_file) {
        input_file.read(reinterpret_cast<char*>(buffer.data()),
                        static_cast<std::streamsize>(buffer.size()));
        const std::streamsize bytes_read = input_file.gcount();
        if (bytes_read > 0) {
            const auto data =
                std::span<const std::uint8_t>(buffer.data(), static_cast<std::size_t>(bytes_read));
            const auto emitted = hashing_chunker.consume(data);
            hashed_chunks.insert(hashed_chunks.end(), emitted.begin(), emitted.end());
        }
    }
    //hashing_chunker.finish();
    if (const auto final_chunk = hashing_chunker.finish()) {
        hashed_chunks.push_back(*final_chunk);
    }
    if (input_file.bad()) {
        throw std::runtime_error("Error reading file: " + path.string());
    }

    return hashed_chunks;
}

} // namespace xet::cdc
