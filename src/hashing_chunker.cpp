#include "xet_cdc/hashing_chunker.hpp"

namespace xet::cdc {

std::vector<HashedChunk>
HashingChunker::consume(std::span<const std::uint8_t> data) {
    pending_.insert(pending_.end(), data.begin(), data.end());

    const std::vector<ChunkBoundary> boundaries =
        chunker_.consume(data);

    std::vector<HashedChunk> hashed_chunks;

    for (const ChunkBoundary& boundary : boundaries) {
        const auto chunk_data =
            std::span<const std::uint8_t>(
                pending_.data(),
                boundary.size);

        hashed_chunks.push_back(HashedChunk{
            .boundary = boundary,
            .hash = hash_chunk(chunk_data),
        });

        pending_.erase(
            pending_.begin(),
            pending_.begin() + boundary.size);
    }

    return hashed_chunks;
}

std::optional<HashedChunk> HashingChunker::finish() {
    std::optional<ChunkBoundary> boundary = chunker_.finish();

    if (boundary) {
        HashedChunk hashed_chunk;
        hashed_chunk.boundary = *boundary;
        hashed_chunk.hash = hash_chunk(std::span<const std::uint8_t>(pending_.data(), pending_.size()));
        pending_.clear();
        return hashed_chunk;
    }

    return std::nullopt;
}

} // namespace xet::cdc
