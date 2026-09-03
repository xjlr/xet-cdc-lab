#include "xet_cdc/chunk_hash.hpp"
#include <blake3.h>

namespace xet::cdc {

namespace {
// The fixed 32-byte key the Xet protocol uses for chunk-level hashing. It is
// an implementation detail of hash_chunk() and is deliberately not exported.
// https://huggingface.co/docs/xet/hashing
constexpr std::array<std::uint8_t, 32> kDataKey = {
    0x66, 0x97, 0xf5, 0x77, 0x5b, 0x95, 0x50, 0xde, 0x31, 0x35, 0xcb, 0xac, 0xa5, 0x97, 0x18, 0x1c,
    0x9d, 0xe4, 0x21, 0x10, 0x9b, 0xeb, 0x2b, 0x58, 0xb4, 0xd0, 0xb0, 0x4b, 0x93, 0xad, 0xf2, 0x29,
};
} // namespace

ChunkHash hash_chunk(std::span<const std::uint8_t> data) {
    ChunkHash hash;

    blake3_hasher hasher;
    blake3_hasher_init_keyed(&hasher, kDataKey.data());
    blake3_hasher_update(&hasher, data.data(), data.size());
    blake3_hasher_finalize(&hasher, hash.bytes.data(), hash.bytes.size());
    return hash;
}

} // namespace xet::cdc
