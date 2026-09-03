#include "xet_cdc/reference_manifest.hpp"

#include <stdexcept>
#include <charconv>
#include <fstream>
#include <istream>
#include <sstream>
#include <string>
#include <system_error>
#include <vector>

namespace xet::cdc {

std::vector<ReferenceChunk>
parse_reference_manifest(std::istream& input) {
    if (!input) {
        throw std::runtime_error("Input stream is not valid");
    }

    std::vector<ReferenceChunk> chunks;
    std::string line;
    std::size_t line_number = 0;

    while (std::getline(input, line)) {
        ++line_number;

        // Remove CR left by CRLF line endings.
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        // Skip empty or whitespace-only lines.
        if (line.find_first_not_of(" \t") == std::string::npos) {
            continue;
        }

        std::istringstream line_stream(line);

        std::string hash_text;
        std::string size_text;
        std::string extra;

        if (!(line_stream >> hash_text >> size_text)) {
            throw std::runtime_error(
                "Malformed reference manifest at line " +
                std::to_string(line_number));
        }

        if (line_stream >> extra) {
            throw std::runtime_error(
                "Unexpected extra field in reference manifest at line " +
                std::to_string(line_number));
        }

        std::uint32_t size = 0;

        const char* first = size_text.data();
        const char* last = first + size_text.size();

        const auto [ptr, ec] =
            std::from_chars(first, last, size);

        if (ec != std::errc{} || ptr != last) {
            throw std::runtime_error(
                "Invalid chunk size at line " +
                std::to_string(line_number) +
                ": " + size_text);
        }

        ChunkHash hash;
        try {
            hash = parse_xet_hash(hash_text);
        } catch (const std::runtime_error& e) {
            throw std::runtime_error(
                "Invalid hash at line " +
                std::to_string(line_number) +
                ": " + e.what());
        }

        chunks.push_back(ReferenceChunk{
            .hash = hash,
            .size = size,
        });
    }

    return chunks;
}

std::vector<ReferenceChunk> load_reference_manifest(const std::filesystem::path& path) {
    std::ifstream input_file(path);
    if (!input_file) {
        throw std::runtime_error("Error opening reference manifest: " + path.string());
    }

    std::vector<ReferenceChunk> chunks = parse_reference_manifest(input_file);

    if (input_file.bad()) {
        throw std::runtime_error("Error reading reference manifest: " + path.string());
    }

    return chunks;
}

} // namespace xet::cdc
