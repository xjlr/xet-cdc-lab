#pragma once

#include "xet_cdc/chunk_hash.hpp"

#include <cstdint>
#include <filesystem>
#include <iosfwd>
#include <vector>

namespace xet::cdc {

// One entry of Hugging Face's `.chunks` manifest: the published chunk hash and
// the chunk length it covers.
struct ReferenceChunk {
    ChunkHash hash{};
    std::uint32_t size{};

    friend constexpr bool operator==(const ReferenceChunk&, const ReferenceChunk&) = default;
};

// Parses Hugging Face's `.chunks` manifest. Every non-blank line holds a chunk
// hash and a decimal chunk length separated by whitespace:
//
//     b10aa1dc71c61661de92280c41a188aabc47981739b785724a099945d8dc5ce4 131072
//
// The entries are returned in manifest order. Blank lines are ignored, a
// trailing carriage return is stripped so Windows line endings parse, and
// whitespace around the two fields is tolerated. Every other malformed line is
// rejected: a line must hold exactly two fields, the hash must be a canonical
// hash string as accepted by parse_xet_hash(), and the length must be a decimal
// number that fits the protocol chunk size type.
//
// Throws std::runtime_error on a malformed line.
[[nodiscard]] std::vector<ReferenceChunk> parse_reference_manifest(std::istream& input);

// Same as above, reading the manifest from a file.
//
// Throws std::runtime_error if the file cannot be opened or read.
[[nodiscard]] std::vector<ReferenceChunk>
load_reference_manifest(const std::filesystem::path& path);

} // namespace xet::cdc
