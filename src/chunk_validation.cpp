#include "xet_cdc/chunk_validation.hpp"

#include <algorithm>

namespace xet::cdc {

std::optional<ChunkSizeMismatchReport>
compare_chunk_sizes(std::span<const ChunkBoundary> actual,
                    std::span<const std::uint32_t> expected) {
    const std::size_t shared_count = std::min(actual.size(), expected.size());

    for (std::size_t index = 0; index < shared_count; ++index) {
        if (actual[index].size != expected[index]) {
            return ChunkSizeMismatch{index, expected[index], actual[index].size};
        }
    }

    if (actual.size() != expected.size()) {
        return ChunkCountMismatch{expected.size(), actual.size()};
    }

    return std::nullopt;
}

} // namespace xet::cdc
