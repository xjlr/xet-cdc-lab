#include "xet_cdc/reference_manifest.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <cstdint>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using namespace xet::cdc;

[[nodiscard]] std::vector<std::uint32_t> parse(const std::string& text) {
    std::istringstream input(text);
    return parse_reference_chunk_sizes(input);
}

TEST_CASE("Reference manifest parser reads hash and size pairs", "[reference_manifest]") {
    const std::string manifest =
        "b10aa1dc71c61661de92280c41a188aabc47981739b785724a099945d8dc5ce4 131072\n"
        "26255591fa803b6baf25d88c315b8a6f5153d5bcfdf18ec5ef526264e0ccc907 106099\n"
        "099cb228194fe640e36a6c7d274ee5ed3a714ccd557a0951d9b6b43a7292b5d1 61389\n";

    const std::vector<std::uint32_t> expected{131072, 106099, 61389};

    REQUIRE(parse(manifest) == expected);
}

TEST_CASE("Reference manifest parser accepts an empty manifest", "[reference_manifest]") {
    REQUIRE(parse("").empty());
}

TEST_CASE("Reference manifest parser ignores blank lines and carriage returns",
          "[reference_manifest]") {
    const std::string manifest = "aa 8192\r\n"
                                 "\n"
                                 "   \n"
                                 "bb 4096\r\n";

    const std::vector<std::uint32_t> expected{8192, 4096};

    REQUIRE(parse(manifest) == expected);
}

TEST_CASE("Reference manifest parser tolerates extra whitespace between fields",
          "[reference_manifest]") {
    const std::vector<std::uint32_t> expected{8192};

    REQUIRE(parse("  aa\t 8192  \n") == expected);
}

TEST_CASE("Reference manifest parser rejects a line without a size", "[reference_manifest]") {
    REQUIRE_THROWS_AS(parse("b10aa1dc\n"), std::runtime_error);
}

TEST_CASE("Reference manifest parser rejects a trailing field", "[reference_manifest]") {
    REQUIRE_THROWS_AS(parse("aa 8192 bb\n"), std::runtime_error);
}

TEST_CASE("Reference manifest parser rejects a non-numeric size", "[reference_manifest]") {
    REQUIRE_THROWS_AS(parse("aa eight\n"), std::runtime_error);
    REQUIRE_THROWS_AS(parse("aa 8192x\n"), std::runtime_error);
    REQUIRE_THROWS_AS(parse("aa -1\n"), std::runtime_error);
}

TEST_CASE("Reference manifest parser rejects a size that does not fit the protocol type",
          "[reference_manifest]") {
    REQUIRE_THROWS_AS(parse("aa 4294967296\n"), std::runtime_error);
}

TEST_CASE("Reference manifest parser reports the failing line number", "[reference_manifest]") {
    const std::string manifest = "aa 8192\n"
                                 "bb 4096\n"
                                 "cc oops\n";

    REQUIRE_THROWS_WITH(parse(manifest), Catch::Matchers::ContainsSubstring("line 3"));
}

} // namespace
