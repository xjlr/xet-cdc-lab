#include "xet_cdc/chunk_validation.hpp"

namespace xet::cdc {

std::optional<ChunkValidationMismatch> compare_chunks(std::span<const HashedChunk> actual,
                                                      std::span<const ReferenceChunk> expected) {
    // TODO: implement the comparison.
    //
    // Walk the shared prefix of the two sequences. For each index, report a
    // ChunkSizeMismatch when the sizes differ, otherwise a ChunkHashMismatch
    // when the hashes differ. If the whole shared prefix matched but the
    // sequences have different lengths, report a ChunkCountMismatch. Otherwise
    // return std::nullopt.
    if (actual.size() != expected.size()) {
        return ChunkCountMismatch{expected.size(), actual.size()};
    }
    for (std::size_t i = 0; i < actual.size(); ++i) {
        if (actual[i].boundary.size != expected[i].size) {
            return ChunkSizeMismatch{i, expected[i].size, actual[i].boundary.size};
        }
        if (actual[i].hash != expected[i].hash) {
            return ChunkHashMismatch{i, expected[i].hash, actual[i].hash};
        }
    }
    return std::nullopt;
}

} // namespace xet::cdc
