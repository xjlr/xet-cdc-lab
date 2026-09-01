#pragma once

#include "xet_cdc/gear_hash.hpp"
#include "xet_cdc/protocol.hpp"

#include <cstdint>
#include <optional>

namespace xet::cdc {

class Chunker {
  public:
    Chunker() noexcept = default;

    [[nodiscard]] std::optional<ChunkBoundary> update(std::uint8_t byte) noexcept;
    [[nodiscard]] std::optional<ChunkBoundary> finish() noexcept;

  private:
    GearHash hash_;

    std::uint64_t chunk_offset_{};
    std::uint32_t chunk_size_{};
};

} // namespace xet::cdc
