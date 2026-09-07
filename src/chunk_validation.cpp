#include "xet_cdc/chunk_validation.hpp"

namespace xet::cdc {

std::optional<ChunkValidationMismatch> compare_chunks(std::span<const HashedChunk> actual,
                                                      std::span<const ReferenceChunk> expected) {
    const std::size_t common =
        std::min(actual.size(), expected.size());

    for (std::size_t i = 0; i < common; ++i) {
        if (actual[i].boundary.size != expected[i].size) {
            return ChunkSizeMismatch{
                i,
                expected[i].size,
                actual[i].boundary.size,
            };
        }

        if (actual[i].hash != expected[i].hash) {
            return ChunkHashMismatch{
                i,
                expected[i].hash,
                actual[i].hash,
            };
        }
    }

    if (actual.size() != expected.size()) {
        return ChunkCountMismatch{
            expected.size(),
            actual.size(),
        };
    }

    return std::nullopt;
}

} // namespace xet::cdc
