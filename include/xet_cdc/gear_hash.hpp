#pragma once

#include <cstdint>

namespace xet::cdc {

class GearHash {
  public:
    GearHash() noexcept = default;

    void update(std::uint8_t byte) noexcept;

    [[nodiscard]] std::uint64_t value() const noexcept { return value_; }

    void reset() noexcept { value_ = 0; }

  private:
    std::uint64_t value_{};
};

} // namespace xet::cdc
