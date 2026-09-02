#include "xet_cdc/chunk_validation.hpp"
#include "xet_cdc/file_chunker.hpp"
#include "xet_cdc/protocol.hpp"
#include "xet_cdc/reference_manifest.hpp"

#include <cstdint>
#include <exception>
#include <iostream>
#include <string_view>
#include <variant>
#include <vector>

namespace {

using namespace xet::cdc;

void print_usage(std::ostream& output) {
    output << "xet-cdc-lab\n\n"
              "Usage:\n"
              "  xet-cdc chunk <file>\n"
              "  xet-cdc validate <file> <reference.chunks>\n"
              "  xet-cdc compare <original> <modified>\n\n"
              "Commands will be implemented one at a time.\n";
}

int run_chunk(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: xet-cdc chunk <file>\n";
        return 1;
    }

    const auto boundaries = chunk_file(argv[2]);

    for (const auto& boundary : boundaries) {
        std::cout << "Chunk: offset=" << boundary.offset << ", size=" << boundary.size << "\n";
    }

    return 0;
}

int run_validate(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: xet-cdc validate <file> <reference.chunks>\n";
        return 1;
    }

    const std::vector<ChunkBoundary> boundaries = chunk_file(argv[2]);
    const std::vector<std::uint32_t> expected_sizes = load_reference_chunk_sizes(argv[3]);

    const auto mismatch = compare_chunk_sizes(boundaries, expected_sizes);

    if (!mismatch) {
        std::cout << "Validation successful: " << boundaries.size() << " chunks matched.\n";
        return 0;
    }

    if (const auto* size_mismatch = std::get_if<ChunkSizeMismatch>(&*mismatch)) {
        std::cerr << "Validation failed at chunk " << size_mismatch->index << ":\n"
                  << "  expected size: " << size_mismatch->expected_size << "\n"
                  << "  actual size:   " << size_mismatch->actual_size << "\n";
        return 1;
    }

    const auto& count_mismatch = std::get<ChunkCountMismatch>(*mismatch);

    std::cerr << "Validation failed: chunk count mismatch:\n"
              << "  expected chunks: " << count_mismatch.expected_count << "\n"
              << "  actual chunks:   " << count_mismatch.actual_count << "\n";
    return 1;
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc == 1 || std::string_view{argv[1]} == "--help" || std::string_view{argv[1]} == "-h") {
        print_usage(std::cout);
        return 0;
    }

    const std::string_view command{argv[1]};

    try {
        if (command == "chunk") {
            return run_chunk(argc, argv);
        }

        if (command == "validate") {
            return run_validate(argc, argv);
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << "\n";
        return 1;
    }

    std::cerr << "Command not implemented yet: " << command << "\n\n";
    print_usage(std::cerr);
    return 2;
}
