#include "xet_cdc/reference_manifest.hpp"

#include <charconv>
#include <cstddef>
#include <fstream>
#include <istream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <system_error>

namespace xet::cdc {
namespace {

[[nodiscard]] bool is_blank(const std::string& line) {
    return line.find_first_not_of(" \t") == std::string::npos;
}

[[nodiscard]] std::runtime_error malformed_line(std::size_t line_number,
                                                const std::string& reason) {
    return std::runtime_error("Malformed manifest line " + std::to_string(line_number) + ": " +
                              reason);
}

[[nodiscard]] std::uint32_t parse_chunk_size(const std::string& token, std::size_t line_number) {
    std::uint32_t size = 0;

    const char* const first = token.data();
    const char* const last = first + token.size();

    const auto [pointer, error] = std::from_chars(first, last, size);

    if (error == std::errc::result_out_of_range) {
        throw malformed_line(line_number, "chunk size out of range: " + token);
    }

    if (error != std::errc{} || pointer != last) {
        throw malformed_line(line_number, "invalid chunk size: " + token);
    }

    return size;
}

} // namespace

std::vector<std::uint32_t> parse_reference_chunk_sizes(std::istream& input) {
    std::vector<std::uint32_t> sizes;

    std::string line;
    std::size_t line_number = 0;

    while (std::getline(input, line)) {
        ++line_number;

        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        if (is_blank(line)) {
            continue;
        }

        std::istringstream fields(line);

        std::string hash;
        std::string size_token;
        std::string trailing;

        if (!(fields >> hash >> size_token)) {
            throw malformed_line(line_number, "expected '<hash> <size>'");
        }

        if (fields >> trailing) {
            throw malformed_line(line_number, "unexpected trailing field: " + trailing);
        }

        sizes.push_back(parse_chunk_size(size_token, line_number));
    }

    return sizes;
}

std::vector<std::uint32_t> load_reference_chunk_sizes(const std::filesystem::path& path) {
    std::ifstream input_file(path);
    if (!input_file) {
        throw std::runtime_error("Error opening reference manifest: " + path.string());
    }

    std::vector<std::uint32_t> sizes = parse_reference_chunk_sizes(input_file);

    if (input_file.bad()) {
        throw std::runtime_error("Error reading reference manifest: " + path.string());
    }

    return sizes;
}

} // namespace xet::cdc
