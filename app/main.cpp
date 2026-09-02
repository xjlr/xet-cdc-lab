#include "xet_cdc/chunker.hpp"
#include "xet_cdc/protocol.hpp"

#include <array>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <span>
#include <string_view>

namespace {

void print_usage(std::ostream& output) {
    output << "xet-cdc-lab\n\n"
              "Usage:\n"
              "  xet-cdc chunk <file>\n"
              "  xet-cdc validate <file> <reference.chunks>\n"
              "  xet-cdc compare <original> <modified>\n\n"
              "Commands will be implemented one at a time.\n";
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc == 1 || std::string_view{argv[1]} == "--help" || std::string_view{argv[1]} == "-h") {
        print_usage(std::cout);
        return 0;
    }

    if (std::string_view{argv[1]} == "chunk") {
        if (argc != 3) {
            std::cerr << "Usage: xet-cdc chunk <file>\n";
            return 1;
        }

        xet::cdc::Chunker chunker;

        std::ifstream input_file(argv[2], std::ios::binary);
        if (!input_file) {
            std::cerr << "Error opening file: " << argv[2] << "\n";
            return 1;
        }

        std::array<std::uint8_t, 64 * 1024> buffer;

        while (input_file) {
            input_file.read(reinterpret_cast<char*>(buffer.data()),
                            static_cast<std::streamsize>(buffer.size()));

            const std::streamsize bytes_read = input_file.gcount();

            if (bytes_read > 0) {
                const auto data = std::span<const std::uint8_t>(
                    buffer.data(), static_cast<std::size_t>(bytes_read));

                const auto boundaries = chunker.consume(data);

                for (const auto& boundary : boundaries) {
                    std::cout << "Chunk: offset=" << boundary.offset << ", size=" << boundary.size
                              << "\n";
                }
            }
        }

        const auto final_boundary = chunker.finish();

        if (final_boundary) {
            std::cout << "Chunk: offset=" << final_boundary->offset
                      << ", size=" << final_boundary->size << "\n";
        }

        return 0;
    }

    std::cerr << "Command not implemented yet: " << argv[1] << "\n\n";
    print_usage(std::cerr);
    return 2;
}
