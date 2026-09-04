#include "xet_cdc/chunk_validation.hpp"

namespace xet::cdc {

std::optional<ChunkValidationMismatch> compare_chunks(std::span<const HashedChunk>,
                                                      std::span<const ReferenceChunk>) {
    // TODO: implement the comparison.
    //
    // Walk the shared prefix of the two sequences. For each index, report a
    // ChunkSizeMismatch when the sizes differ, otherwise a ChunkHashMismatch
    // when the hashes differ. If the whole shared prefix matched but the
    // sequences have different lengths, report a ChunkCountMismatch. Otherwise
    // return std::nullopt.
    return std::nullopt;
}

} // namespace xet::cdc
