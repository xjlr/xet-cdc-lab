#pragma once

#include "xet_cdc/chunk_hash.hpp"
#include "xet_cdc/protocol.hpp"

namespace xet::cdc {

// One CDC chunk together with the keyed BLAKE3 hash of exactly the bytes that
// its boundary covers.
//
// The type joins the two independent layers -- boundary detection and chunk
// hashing -- without making either of them depend on the other, which keeps
// the dependency direction of the project intact.
struct HashedChunk {
    ChunkBoundary boundary{};
    ChunkHash hash{};

    friend constexpr bool operator==(const HashedChunk&, const HashedChunk&) = default;
};

} // namespace xet::cdc
