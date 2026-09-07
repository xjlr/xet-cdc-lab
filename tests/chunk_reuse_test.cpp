#include "xet_cdc/chunk_reuse.hpp"

#include "xet_cdc/chunk_hash.hpp"
#include "xet_cdc/hashed_chunk.hpp"
#include "xet_cdc/protocol.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_tostring.hpp>

#include <array>
#include <cstdint>
#include <string>
#include <vector>

// Without this a failed comparison prints "{?} == {?}", which says nothing
// about which of the eight statistics is wrong.
namespace Catch {

template <> struct StringMaker<xet::cdc::ChunkReuseResult> {
    static std::string convert(const xet::cdc::ChunkReuseResult& result) {
        return "chunks(original=" + std::to_string(result.original_chunk_count) +
               ", modified=" + std::to_string(result.modified_chunk_count) +
               ", reused=" + std::to_string(result.reused_chunk_count) +
               ", new=" + std::to_string(result.new_chunk_count) +
               "), bytes(original=" + std::to_string(result.original_bytes) +
               ", modified=" + std::to_string(result.modified_bytes) +
               ", reused=" + std::to_string(result.reused_bytes) +
               ", new=" + std::to_string(result.new_bytes) + ")";
    }
};

} // namespace Catch

namespace {

using namespace xet::cdc;

// One chunk of a test sequence: its length, plus a seed standing in for its
// content. Two entries hash equal exactly when their seeds are equal, which is
// what lets a test spell out sequences such as A B X C D E.
struct ChunkSpec {
    std::uint32_t size{};
    char seed{};
};

[[nodiscard]] ChunkHash hash_of(char seed) {
    const std::array<std::uint8_t, 1> content{static_cast<std::uint8_t>(seed)};

    return hash_chunk(content);
}

[[nodiscard]] HashedChunk make_chunk(std::uint64_t offset, std::uint32_t size, char seed) {
    return HashedChunk{ChunkBoundary{offset, size}, hash_of(seed)};
}

// Lays the specs out back to back from offset zero, the way a chunker would.
[[nodiscard]] std::vector<HashedChunk> sequence(const std::vector<ChunkSpec>& specs) {
    std::vector<HashedChunk> chunks;

    std::uint64_t offset = 0;

    for (const ChunkSpec& spec : specs) {
        chunks.push_back(make_chunk(offset, spec.size, spec.seed));
        offset += spec.size;
    }

    return chunks;
}

// The two invariants that must hold for every result: each modified chunk is
// counted exactly once, in chunks and in bytes. Expected byte totals are always
// written out explicitly as well, never inferred from a chunk count.
void require_modified_side_adds_up(const ChunkReuseResult& result) {
    REQUIRE(result.reused_chunk_count + result.new_chunk_count == result.modified_chunk_count);
    REQUIRE(result.reused_bytes + result.new_bytes == result.modified_bytes);
}

// Chunk contents used below. Sizes differ per chunk so that byte totals cannot
// accidentally agree with a count-based expectation.
constexpr ChunkSpec kA{131072, 'A'};
constexpr ChunkSpec kB{106099, 'B'};
constexpr ChunkSpec kC{61389, 'C'};
constexpr ChunkSpec kD{40000, 'D'};
constexpr ChunkSpec kE{9000, 'E'};
constexpr ChunkSpec kX{12345, 'X'};
constexpr ChunkSpec kY{80000, 'Y'};
constexpr ChunkSpec kZ{90000, 'Z'};

TEST_CASE("compare_chunk_reuse reports nothing for two empty sequences", "[chunk_reuse]") {
    const auto result = compare_chunk_reuse({}, {});

    REQUIRE(result == ChunkReuseResult{});
    require_modified_side_adds_up(result);
}

TEST_CASE("compare_chunk_reuse reuses every chunk of an unchanged file", "[chunk_reuse]") {
    // A B C  ->  A B C
    const auto original = sequence({kA, kB, kC});
    const auto modified = sequence({kA, kB, kC});

    const auto result = compare_chunk_reuse(original, modified);

    REQUIRE(result == ChunkReuseResult{
                          .original_chunk_count = 3,
                          .modified_chunk_count = 3,
                          .reused_chunk_count = 3,
                          .new_chunk_count = 0,
                          .original_bytes = 298560,
                          .modified_bytes = 298560,
                          .reused_bytes = 298560,
                          .new_bytes = 0,
                      });
    require_modified_side_adds_up(result);
}

TEST_CASE("compare_chunk_reuse reuses nothing when no content is shared", "[chunk_reuse]") {
    // A B C  ->  X Y Z
    const auto original = sequence({kA, kB, kC});
    const auto modified = sequence({kX, kY, kZ});

    const auto result = compare_chunk_reuse(original, modified);

    REQUIRE(result == ChunkReuseResult{
                          .original_chunk_count = 3,
                          .modified_chunk_count = 3,
                          .reused_chunk_count = 0,
                          .new_chunk_count = 3,
                          .original_bytes = 298560,
                          .modified_bytes = 182345,
                          .reused_bytes = 0,
                          .new_bytes = 182345,
                      });
    require_modified_side_adds_up(result);
}

TEST_CASE("compare_chunk_reuse charges an insertion for the inserted chunk only", "[chunk_reuse]") {
    // A B C D E  ->  A B X C D E. The chunks after the insertion keep their
    // content but move to new offsets; all of them must stay reusable.
    const auto original = sequence({kA, kB, kC, kD, kE});
    const auto modified = sequence({kA, kB, kX, kC, kD, kE});

    const auto result = compare_chunk_reuse(original, modified);

    REQUIRE(result == ChunkReuseResult{
                          .original_chunk_count = 5,
                          .modified_chunk_count = 6,
                          .reused_chunk_count = 5,
                          .new_chunk_count = 1,
                          .original_bytes = 347560,
                          .modified_bytes = 359905,
                          .reused_bytes = 347560,
                          .new_bytes = 12345,
                      });
    require_modified_side_adds_up(result);
}

TEST_CASE("compare_chunk_reuse reuses everything that survives a deletion", "[chunk_reuse]") {
    // A B C D  ->  A C D
    const auto original = sequence({kA, kB, kC, kD});
    const auto modified = sequence({kA, kC, kD});

    const auto result = compare_chunk_reuse(original, modified);

    REQUIRE(result == ChunkReuseResult{
                          .original_chunk_count = 4,
                          .modified_chunk_count = 3,
                          .reused_chunk_count = 3,
                          .new_chunk_count = 0,
                          .original_bytes = 338560,
                          .modified_bytes = 232461,
                          .reused_bytes = 232461,
                          .new_bytes = 0,
                      });
    require_modified_side_adds_up(result);
}

TEST_CASE("compare_chunk_reuse is not positional", "[chunk_reuse]") {
    // A B C  ->  C A B. Nothing lines up index for index, yet every chunk's
    // content exists in the original.
    const auto original = sequence({kA, kB, kC});
    const auto modified = sequence({kC, kA, kB});

    const auto result = compare_chunk_reuse(original, modified);

    REQUIRE(result == ChunkReuseResult{
                          .original_chunk_count = 3,
                          .modified_chunk_count = 3,
                          .reused_chunk_count = 3,
                          .new_chunk_count = 0,
                          .original_bytes = 298560,
                          .modified_bytes = 298560,
                          .reused_bytes = 298560,
                          .new_bytes = 0,
                      });
    require_modified_side_adds_up(result);
}

TEST_CASE("compare_chunk_reuse is not limited by the original's multiplicity", "[chunk_reuse]") {
    // A A B  ->  A A A B. Content existence decides reuse, so the third copy of
    // A is reusable even though the original holds only two.
    const auto original = sequence({kA, kA, kB});
    const auto modified = sequence({kA, kA, kA, kB});

    const auto result = compare_chunk_reuse(original, modified);

    REQUIRE(result == ChunkReuseResult{
                          .original_chunk_count = 3,
                          .modified_chunk_count = 4,
                          .reused_chunk_count = 4,
                          .new_chunk_count = 0,
                          .original_bytes = 368243,
                          .modified_bytes = 499315,
                          .reused_bytes = 499315,
                          .new_bytes = 0,
                      });
    require_modified_side_adds_up(result);
}

TEST_CASE("compare_chunk_reuse does not match on size", "[chunk_reuse]") {
    // Same length, different content: the modified chunk is new.
    const std::vector<HashedChunk> original{make_chunk(0, 131072, 'A')};
    const std::vector<HashedChunk> modified{make_chunk(0, 131072, 'Q')};

    const auto result = compare_chunk_reuse(original, modified);

    REQUIRE(result == ChunkReuseResult{
                          .original_chunk_count = 1,
                          .modified_chunk_count = 1,
                          .reused_chunk_count = 0,
                          .new_chunk_count = 1,
                          .original_bytes = 131072,
                          .modified_bytes = 131072,
                          .reused_bytes = 0,
                          .new_bytes = 131072,
                      });
    require_modified_side_adds_up(result);
}

TEST_CASE("compare_chunk_reuse ignores chunk offsets", "[chunk_reuse]") {
    // Same content and length, unrelated offsets on both sides.
    const std::vector<HashedChunk> original{make_chunk(4'000'000, 131072, 'A')};
    const std::vector<HashedChunk> modified{make_chunk(0, 131072, 'A')};

    const auto result = compare_chunk_reuse(original, modified);

    REQUIRE(result == ChunkReuseResult{
                          .original_chunk_count = 1,
                          .modified_chunk_count = 1,
                          .reused_chunk_count = 1,
                          .new_chunk_count = 0,
                          .original_bytes = 131072,
                          .modified_bytes = 131072,
                          .reused_bytes = 131072,
                          .new_bytes = 0,
                      });
    require_modified_side_adds_up(result);
}

TEST_CASE("compare_chunk_reuse totals bytes per chunk, not per chunk count", "[chunk_reuse]") {
    // The reused chunks are the small one and the large one, and the single new
    // chunk is smaller than either: only exact per-chunk sizes give the right
    // byte totals here.
    const auto original = sequence({kA, kE, kC});
    const auto modified = sequence({kE, kX, kC});

    const auto result = compare_chunk_reuse(original, modified);

    REQUIRE(result == ChunkReuseResult{
                          .original_chunk_count = 3,
                          .modified_chunk_count = 3,
                          .reused_chunk_count = 2,
                          .new_chunk_count = 1,
                          .original_bytes = 201461,
                          .modified_bytes = 82734,
                          .reused_bytes = 70389,
                          .new_bytes = 12345,
                      });
    require_modified_side_adds_up(result);
}

TEST_CASE("compare_chunk_reuse treats every chunk as new when the original is empty",
          "[chunk_reuse]") {
    const auto modified = sequence({kA, kB});

    const auto result = compare_chunk_reuse({}, modified);

    REQUIRE(result == ChunkReuseResult{
                          .original_chunk_count = 0,
                          .modified_chunk_count = 2,
                          .reused_chunk_count = 0,
                          .new_chunk_count = 2,
                          .original_bytes = 0,
                          .modified_bytes = 237171,
                          .reused_bytes = 0,
                          .new_bytes = 237171,
                      });
    require_modified_side_adds_up(result);
}

TEST_CASE("compare_chunk_reuse reports an empty modified file as nothing to store",
          "[chunk_reuse]") {
    const auto original = sequence({kA, kB, kC});

    const auto result = compare_chunk_reuse(original, {});

    REQUIRE(result == ChunkReuseResult{
                          .original_chunk_count = 3,
                          .modified_chunk_count = 0,
                          .reused_chunk_count = 0,
                          .new_chunk_count = 0,
                          .original_bytes = 298560,
                          .modified_bytes = 0,
                          .reused_bytes = 0,
                          .new_bytes = 0,
                      });
    require_modified_side_adds_up(result);
}

} // namespace
