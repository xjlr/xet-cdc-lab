#pragma once

#include "xet_cdc/gear_hash.hpp"
#include "xet_cdc/protocol.hpp"

#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace xet::cdc {

class Chunker {
  public:
    Chunker() noexcept = default;

    [[nodiscard]] std::vector<ChunkBoundary> consume(std::span<const std::uint8_t> data);

    [[nodiscard]] std::optional<ChunkBoundary> finish() noexcept;

  private:
    [[nodiscard]] std::optional<ChunkBoundary> update(std::uint8_t byte) noexcept;

    GearHash hash_;

    std::uint64_t chunk_offset_{};
    std::uint32_t chunk_size_{};
};

} // namespace xet::cdc
