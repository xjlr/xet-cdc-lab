#include "xet_cdc/chunk_hash.hpp"
#include <blake3.h>

#include <charconv>
#include <stdexcept>

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

std::string to_xet_hex(const ChunkHash& hash) {
    static constexpr char hex_digits[] = "0123456789abcdef";

    std::string result;
    result.reserve(64);

    for (std::size_t block = 0; block < 4; ++block) {
        for (std::size_t j = 0; j < 8; ++j) {
            const std::uint8_t byte =
                hash.bytes[block * 8 + (7 - j)];

            result.push_back(hex_digits[byte >> 4]);
            result.push_back(hex_digits[byte & 0x0F]);
        }
    }

    return result;
}

ChunkHash parse_xet_hash(std::string_view text) {
    // Validate the input length
    if (text.size() != 64) {
        throw std::runtime_error("Invalid hash string length");
    }

    // Validate that all characters are lowercase hex digits
    for (char c : text) {
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) {
            throw std::runtime_error("Invalid character in hash string");
        }
    }              
   
    ChunkHash hash;
    for (std::size_t i = 0; i < 32; ++i) {
        const std::size_t dst =
            8 * (i / 8) + (7 - i % 8);

        unsigned int value = 0;

        const char* first = text.data() + 2 * i;
        const char* last = first + 2;

        const auto [ptr, ec] =
            std::from_chars(first, last, value, 16);

        if (ec != std::errc{} || ptr != last) {
            throw std::runtime_error("Invalid hash string");
        }

        hash.bytes[dst] = static_cast<std::uint8_t>(value);
    }

    return hash;
}

} // namespace xet::cdc
