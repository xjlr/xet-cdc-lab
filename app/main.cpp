#include "xet_cdc/protocol.hpp"

#include <iostream>
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
    if (argc == 1 || std::string_view{argv[1]} == "--help" ||
        std::string_view{argv[1]} == "-h") {
        print_usage(std::cout);
        return 0;
    }

    std::cerr << "Command not implemented yet: " << argv[1] << "\n\n";
    print_usage(std::cerr);
    return 2;
}

