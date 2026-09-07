#include "xet_cdc/chunk_reuse.hpp"

#include <unordered_set>

namespace xet::cdc {

ChunkReuseResult compare_chunk_reuse(std::span<const HashedChunk> original, std::span<const HashedChunk> modified) {
    std::size_t original_chunk_count = 0;
    std::size_t modified_chunk_count = 0;
    std::size_t reused_chunk_count = 0;
    std::size_t new_chunk_count = 0;
    std::uint64_t original_bytes = 0;
    std::uint64_t modified_bytes = 0;

    std::uint64_t reused_bytes = 0;
    std::uint64_t new_bytes = 0;

    std::unordered_set<ChunkHash, ChunkHashHasher> original_hashes;
    original_hashes.reserve(original.size());

    for (const auto& chunk : original) {
        ++original_chunk_count;
        original_bytes += chunk.boundary.size;
        original_hashes.insert(chunk.hash);
    }

    for (const auto& chunk : modified) {
        ++modified_chunk_count;
        modified_bytes += chunk.boundary.size;

        if (original_hashes.find(chunk.hash) != original_hashes.end()) {
            ++reused_chunk_count;
            reused_bytes += chunk.boundary.size;
        } else {
            ++new_chunk_count;
            new_bytes += chunk.boundary.size;
        }
    }

    ChunkReuseResult result{
        .original_chunk_count = original_chunk_count,
        .modified_chunk_count = modified_chunk_count,
        .reused_chunk_count = reused_chunk_count,
        .new_chunk_count = new_chunk_count,
        .original_bytes = original_bytes,
        .modified_bytes = modified_bytes,
        .reused_bytes = reused_bytes,
        .new_bytes = new_bytes,
    };

    return result;
}

} // namespace xet::cdc
