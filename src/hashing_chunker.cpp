#include "xet_cdc/hashing_chunker.hpp"

namespace xet::cdc {

std::vector<HashedChunk> HashingChunker::consume(std::span<const std::uint8_t> data) {
    // TODO: implement the streaming hash/boundary coordination.
    //
    // Append `data` to pending_, then forward the same bytes to chunker_. For
    // every boundary chunker_ emits, in order, the chunk's bytes are the first
    // boundary.size bytes still left in pending_: hash exactly that prefix with
    // hash_chunk(), emit the HashedChunk, and erase the prefix so the next
    // boundary starts at the front again. Whatever remains in pending_ after
    // the loop belongs to the chunk still being accumulated.
    static_cast<void>(data);
    static_cast<void>(chunker_);
    static_cast<void>(pending_);

    return {};
}

std::optional<HashedChunk> HashingChunker::finish() {
    // TODO: take the final boundary from chunker_.finish() and hash the bytes
    // still pending, which are exactly that chunk's bytes.
    return std::nullopt;
}

} // namespace xet::cdc
