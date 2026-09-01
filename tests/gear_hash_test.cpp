#include "xet_cdc/gear_hash.hpp"

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <string_view>

namespace {

using xet::cdc::GearHash;

TEST_CASE("GearHash starts at zero and can be reset", "[gear_hash]") {
    GearHash hash;

    REQUIRE(hash.value() == 0);

    hash.update(0);
    REQUIRE(hash.value() == 0xb088d3a9e840f559ULL);

    hash.reset();
    REQUIRE(hash.value() == 0);

    // The object should produce the same result after being reset.
    hash.update(0);
    REQUIRE(hash.value() == 0xb088d3a9e840f559ULL);
}

TEST_CASE("GearHash updates its state incrementally", "[gear_hash]") {
    GearHash hash;

    hash.update(0);
    REQUIRE(hash.value() == 0xb088d3a9e840f559ULL);

    hash.update(1);
    REQUIRE(hash.value() == 0xb7646f4b0a6f0b88ULL);
}

TEST_CASE("GearHash matches a known text vector", "[gear_hash]") {
    constexpr std::string_view input{"hello"};

    GearHash hash;

    for (const char byte : input) {
        hash.update(static_cast<std::uint8_t>(byte));
    }

    REQUIRE(hash.value() == 0xc3dbfa67038de097ULL);
}

TEST_CASE("GearHash handles every possible byte value", "[gear_hash]") {
    GearHash hash;

    for (std::uint16_t byte = 0; byte < 256; ++byte) {
        hash.update(static_cast<std::uint8_t>(byte));
    }

    REQUIRE(hash.value() == 0xd5bc3856d8dfcdebULL);
}

} // namespace