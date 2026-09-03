#include "xet_cdc/reference_manifest.hpp"

#include "xet_cdc/chunk_hash.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <ios>
#include <sstream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

namespace {

using namespace xet::cdc;

// The first three entries of the official reference manifest for
// reference-data/Electric_Vehicle_Population_Data_20250917.csv.
const std::string kHash0 = "b10aa1dc71c61661de92280c41a188aabc47981739b785724a099945d8dc5ce4";
const std::string kHash1 = "26255591fa803b6baf25d88c315b8a6f5153d5bcfdf18ec5ef526264e0ccc907";
const std::string kHash2 = "099cb228194fe640e36a6c7d274ee5ed3a714ccd557a0951d9b6b43a7292b5d1";

constexpr std::uint32_t kSize0 = 131072;
constexpr std::uint32_t kSize1 = 106099;
constexpr std::uint32_t kSize2 = 61389;

// Expected hashes are built with parse_xet_hash(), which chunk_hash_test.cpp
// covers independently of this parser.
[[nodiscard]] ReferenceChunk chunk_of(const std::string& hash, std::uint32_t size) {
    return ReferenceChunk{parse_xet_hash(hash), size};
}

[[nodiscard]] std::vector<ReferenceChunk> parse(const std::string& text) {
    std::istringstream input(text);
    return parse_reference_manifest(input);
}

// Writes a temporary manifest for a single test case and removes it afterwards.
class TemporaryManifest {
  public:
    explicit TemporaryManifest(const std::string& text, const std::string& name)
        : path_(std::filesystem::temp_directory_path() / ("xet_cdc_test_" + name)) {
        std::ofstream output(path_, std::ios::trunc);
        output << text;
    }

    TemporaryManifest(const TemporaryManifest&) = delete;
    TemporaryManifest& operator=(const TemporaryManifest&) = delete;

    ~TemporaryManifest() {
        std::error_code error;
        std::filesystem::remove(path_, error);
    }

    [[nodiscard]] const std::filesystem::path& path() const noexcept { return path_; }

  private:
    std::filesystem::path path_;
};

// The entry is exactly a hash and a size, with no padding or extra members.
static_assert(sizeof(ReferenceChunk) >= sizeof(ChunkHash) + sizeof(std::uint32_t));
static_assert(ReferenceChunk{} == ReferenceChunk{});

TEST_CASE("Reference manifest parser reads a single entry", "[reference_manifest]") {
    const std::vector<ReferenceChunk> chunks = parse(kHash0 + " 131072\n");

    REQUIRE(chunks.size() == 1);
    REQUIRE(chunks.front().size == kSize0);
    REQUIRE(chunks.front().hash == parse_xet_hash(kHash0));
}

TEST_CASE("Reference manifest parser reads entries in manifest order", "[reference_manifest]") {
    const std::string manifest = kHash0 + " 131072\n" + kHash1 + " 106099\n" + kHash2 + " 61389\n";

    const std::vector<ReferenceChunk> expected{
        chunk_of(kHash0, kSize0),
        chunk_of(kHash1, kSize1),
        chunk_of(kHash2, kSize2),
    };

    REQUIRE(parse(manifest) == expected);
}

TEST_CASE("Reference manifest parser accepts a manifest without a final newline",
          "[reference_manifest]") {
    const std::vector<ReferenceChunk> expected{chunk_of(kHash0, kSize0)};

    REQUIRE(parse(kHash0 + " 131072") == expected);
}

TEST_CASE("Reference manifest parser accepts an empty manifest", "[reference_manifest]") {
    REQUIRE(parse("").empty());
}

TEST_CASE("Reference manifest parser ignores blank lines", "[reference_manifest]") {
    const std::string manifest = "\n"
                                 "   \n" +
                                 kHash0 + " 131072\n" + "\n" + "\t\n" + kHash1 + " 106099\n" +
                                 "\n" + "  \n";

    const std::vector<ReferenceChunk> expected{
        chunk_of(kHash0, kSize0),
        chunk_of(kHash1, kSize1),
    };

    REQUIRE(parse(manifest) == expected);
}

TEST_CASE("Reference manifest parser accepts Windows line endings", "[reference_manifest]") {
    const std::string manifest = kHash0 + " 131072\r\n" + kHash1 + " 106099\r\n";

    const std::vector<ReferenceChunk> expected{
        chunk_of(kHash0, kSize0),
        chunk_of(kHash1, kSize1),
    };

    REQUIRE(parse(manifest) == expected);
}

TEST_CASE("Reference manifest parser tolerates extra whitespace around the fields",
          "[reference_manifest]") {
    const std::vector<ReferenceChunk> expected{chunk_of(kHash0, kSize0)};

    REQUIRE(parse("  " + kHash0 + "\t 131072  \n") == expected);
}

TEST_CASE("Reference manifest parser rejects a malformed hash", "[reference_manifest]") {
    // One digit short.
    REQUIRE_THROWS_AS(parse(kHash0.substr(1) + " 131072\n"), std::runtime_error);

    // One digit too long.
    REQUIRE_THROWS_AS(parse(kHash0 + "a 131072\n"), std::runtime_error);

    // Non-hex character in an otherwise well-formed hash.
    REQUIRE_THROWS_AS(parse("z" + kHash0.substr(1) + " 131072\n"), std::runtime_error);

    // parse_xet_hash() accepts only the canonical lowercase form.
    std::string uppercase = kHash0;
    uppercase[0] = 'B';
    REQUIRE_THROWS_AS(parse(uppercase + " 131072\n"), std::runtime_error);

    // Not a hash string at all.
    REQUIRE_THROWS_AS(parse("not-a-hash 131072\n"), std::runtime_error);
}

TEST_CASE("Reference manifest parser rejects a malformed size", "[reference_manifest]") {
    REQUIRE_THROWS_AS(parse(kHash0 + " eight\n"), std::runtime_error);
    REQUIRE_THROWS_AS(parse(kHash0 + " 123abc\n"), std::runtime_error);
    REQUIRE_THROWS_AS(parse(kHash0 + " 8192x\n"), std::runtime_error);
    REQUIRE_THROWS_AS(parse(kHash0 + " -1\n"), std::runtime_error);
}

TEST_CASE("Reference manifest parser rejects a size that does not fit the protocol type",
          "[reference_manifest]") {
    REQUIRE_THROWS_AS(parse(kHash0 + " 4294967296\n"), std::runtime_error);
}

TEST_CASE("Reference manifest parser rejects a line without a hash", "[reference_manifest]") {
    REQUIRE_THROWS_AS(parse("131072\n"), std::runtime_error);
}

TEST_CASE("Reference manifest parser rejects a line without a size", "[reference_manifest]") {
    REQUIRE_THROWS_AS(parse(kHash0 + "\n"), std::runtime_error);
}

TEST_CASE("Reference manifest parser rejects a trailing field", "[reference_manifest]") {
    REQUIRE_THROWS_AS(parse(kHash0 + " 131072 unexpected\n"), std::runtime_error);
}

TEST_CASE("Reference manifest parser reports the failing line number", "[reference_manifest]") {
    const std::string manifest = kHash0 + " 131072\n" + kHash1 + " 106099\n" + kHash2 + " oops\n";

    REQUIRE_THROWS_WITH(parse(manifest), Catch::Matchers::ContainsSubstring("line 3"));
}

TEST_CASE("load_reference_manifest reads a manifest file", "[reference_manifest]") {
    const std::string manifest = kHash0 + " 131072\n" + kHash1 + " 106099\n";

    const TemporaryManifest file(manifest, "reference_manifest_load.chunks");

    const std::vector<ReferenceChunk> expected{
        chunk_of(kHash0, kSize0),
        chunk_of(kHash1, kSize1),
    };

    REQUIRE(load_reference_manifest(file.path()) == expected);
}

TEST_CASE("load_reference_manifest propagates a malformed line", "[reference_manifest]") {
    const TemporaryManifest file(kHash0 + " oops\n", "reference_manifest_malformed.chunks");

    REQUIRE_THROWS_AS(load_reference_manifest(file.path()), std::runtime_error);
}

TEST_CASE("load_reference_manifest throws when the file cannot be opened", "[reference_manifest]") {
    const auto missing = std::filesystem::temp_directory_path() / "xet_cdc_test_missing.chunks";

    std::error_code error;
    std::filesystem::remove(missing, error);

    REQUIRE_THROWS_AS(load_reference_manifest(missing), std::runtime_error);
}

} // namespace
