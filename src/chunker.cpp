#include "xet_cdc/chunker.hpp"

namespace xet::cdc {

std::optional<ChunkBoundary> Chunker::update(std::uint8_t byte) noexcept {
    hash_.update(byte);
    ++chunk_size_;

    if (chunk_size_ < kMinChunkSize) {
        return std::nullopt;
    }

    if (chunk_size_ >= kMaxChunkSize || (hash_.value() & kBoundaryMask) == 0) {
        ChunkBoundary boundary{chunk_offset_, chunk_size_};
        chunk_offset_ += chunk_size_;
        chunk_size_ = 0;
        hash_.reset();
        return boundary;
    }

    return std::nullopt;
}

std::optional<ChunkBoundary> Chunker::finish() noexcept {
    if (chunk_size_ == 0) {
        return std::nullopt;
    }

    ChunkBoundary boundary{chunk_offset_, chunk_size_};
    chunk_offset_ += chunk_size_;
    chunk_size_ = 0;
    hash_.reset();

    return boundary;
}

} // namespace xet::cdc
