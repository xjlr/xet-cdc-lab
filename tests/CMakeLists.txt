include(FetchContent)

# Catch2 v3.15.1. The archive hash makes the downloaded test dependency
# content-addressed and reproducible.
FetchContent_Declare(
    Catch2
    URL https://github.com/catchorg/Catch2/archive/refs/tags/v3.15.1.tar.gz
    URL_HASH SHA256=be23a52b85cf04cd9587612147a10b023d59ed9757fa1843cc99e615d6c0893c
)
FetchContent_MakeAvailable(Catch2)

list(APPEND CMAKE_MODULE_PATH ${Catch2_SOURCE_DIR}/extras)
include(Catch)

add_executable(xet_cdc_tests protocol_constants_test.cpp gear_hash_test.cpp chunker_test.cpp)
target_link_libraries(
    xet_cdc_tests
    PRIVATE
        xet_cdc::xet_cdc
        xet_cdc_warnings
        xet_cdc_sanitizers
        Catch2::Catch2WithMain
)
set_target_properties(xet_cdc_tests PROPERTIES CXX_EXTENSIONS OFF)

catch_discover_tests(xet_cdc_tests)
