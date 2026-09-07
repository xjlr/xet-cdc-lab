#include "xet_cdc/chunk_validation.hpp"
#include "xet_cdc/file_chunker.hpp"
#include "xet_cdc/reference_manifest.hpp"

#include <catch2/catch_test_macros.hpp>

#include <filesystem>

namespace {

using namespace xet::cdc;

// The published reference pair lives in reference-data/, which the README asks
// the developer to download and which .gitignore keeps out of the repository.
// The test therefore skips instead of failing when the files are absent, so a
// clean checkout and CI stay green without ever downloading anything.
constexpr const char* kReferenceFile =
    XET_CDC_REFERENCE_DATA_DIR "/Electric_Vehicle_Population_Data_20250917.csv";

constexpr const char* kReferenceManifest =
    XET_CDC_REFERENCE_DATA_DIR "/Electric_Vehicle_Population_Data_20250917.csv.chunks";

TEST_CASE("hash_file_chunks reproduces the published reference manifest", "[reference]") {
    const std::filesystem::path file{kReferenceFile};
    const std::filesystem::path manifest{kReferenceManifest};

    if (!std::filesystem::exists(file) || !std::filesystem::exists(manifest)) {
        SKIP("reference-data/ is not present; see the README for how to download it");
    }

    const auto expected = load_reference_manifest(manifest);

    REQUIRE(expected.size() == 796);

    const auto actual = hash_file_chunks(file);

    REQUIRE(actual.size() == expected.size());
    REQUIRE_FALSE(compare_chunks(actual, expected).has_value());
}

} // namespace
