#pragma once

#include <cstdint>
#include <filesystem>
#include <iosfwd>
#include <vector>

namespace xet::cdc {

// Parses Hugging Face's `.chunks` manifest. Every non-blank line holds a chunk
// hash and a decimal chunk length separated by whitespace:
//
//     b10aa1dc71c61661de92280c41a188aabc47981739b785724a099945d8dc5ce4 131072
//
// The hash token is skipped for now; only the chunk lengths are returned, in
// manifest order. Blank lines are ignored, every other malformed line is
// rejected.
//
// Throws std::runtime_error on a malformed line or a length that does not fit
// the protocol chunk size type.
[[nodiscard]] std::vector<std::uint32_t> parse_reference_chunk_sizes(std::istream& input);

// Same as above, reading the manifest from a file.
//
// Throws std::runtime_error if the file cannot be opened or read.
[[nodiscard]] std::vector<std::uint32_t>
load_reference_chunk_sizes(const std::filesystem::path& path);

} // namespace xet::cdc
