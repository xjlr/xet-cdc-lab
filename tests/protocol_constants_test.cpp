#include "xet_cdc/protocol.hpp"

#include <cstdlib>
#include <iostream>

int main() {
    using namespace xet::cdc;

    if (kMinChunkSize != 8 * 1024 || kTargetChunkSize != 64 * 1024 ||
        kMaxChunkSize != 128 * 1024 || kBoundaryMask != 0xFFFF'0000'0000'0000ULL) {
        std::cerr << "Xet protocol constants do not match the specification\n";
        return EXIT_FAILURE;
    }

    constexpr ChunkBoundary boundary{.offset = 100, .size = 25};
    static_assert(boundary.end_offset() == 125);
    return EXIT_SUCCESS;
}

