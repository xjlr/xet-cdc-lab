#pragma once

#include "xet_cdc/protocol.hpp"

#include <filesystem>
#include <vector>

namespace xet::cdc {

// Reads the file incrementally and returns every chunk boundary it contains,
// including the final one. Printing and other CLI concerns stay out of here.
//
// Throws std::runtime_error if the file cannot be opened or read.
[[nodiscard]] std::vector<ChunkBoundary> chunk_file(const std::filesystem::path& path);

} // namespace xet::cdc
