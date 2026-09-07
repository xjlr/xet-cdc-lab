cmake_minimum_required(VERSION 3.21)

project(
    xet_cdc_lab
    VERSION 0.1.0
    DESCRIPTION "A C++20 exploration of Hugging Face Xet content-defined chunking"
    # C is enabled for the vendored BLAKE3 implementation. It must be listed
    # here rather than only in that subproject, so that CMake detects the C
    # compiler's features for BLAKE3's target_compile_features(... c_std_99).
    LANGUAGES CXX C
)

option(XET_CDC_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)
option(XET_CDC_BUILD_TESTING "Build the xet-cdc-lab tests" ${PROJECT_IS_TOP_LEVEL})
option(XET_CDC_ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

# Official BLAKE3 C implementation v1.8.2, used for the keyed chunk hash. The
# archive hash makes the downloaded dependency content-addressed and
# reproducible, matching how the tests fetch Catch2.
FetchContent_Declare(
    BLAKE3
    URL https://github.com/BLAKE3-team/BLAKE3/archive/refs/tags/1.8.2.tar.gz
    URL_HASH SHA256=6b51aefe515969785da02e87befafc7fdc7a065cd3458cf1141f29267749e81f
    SOURCE_SUBDIR c
)
FetchContent_MakeAvailable(BLAKE3)

# Warning flags are private project build settings. Consumers of the library
# must not inherit them merely by linking to xet_cdc.
add_library(xet_cdc_warnings INTERFACE)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(
        xet_cdc_warnings
        INTERFACE
            -Wall
            -Wextra
            -Wpedantic
            -Wconversion
            -Wsign-conversion
            -Wshadow
            $<$<BOOL:${XET_CDC_WARNINGS_AS_ERRORS}>:-Werror>
    )
elseif(MSVC)
    target_compile_options(
        xet_cdc_warnings
        INTERFACE
            /W4
            /permissive-
            $<$<BOOL:${XET_CDC_WARNINGS_AS_ERRORS}>:/WX>
    )
endif()

add_library(xet_cdc_sanitizers INTERFACE)

if(XET_CDC_ENABLE_SANITIZERS)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(
            xet_cdc_sanitizers
            INTERFACE
                -fsanitize=address,undefined
                -fno-omit-frame-pointer
                -fno-sanitize-recover=all
        )
        target_link_options(
            xet_cdc_sanitizers
            INTERFACE
                -fsanitize=address,undefined
                -fno-sanitize-recover=all
        )
    else()
        message(FATAL_ERROR "XET_CDC_ENABLE_SANITIZERS requires GCC or Clang")
    endif()
endif()

add_library(
    xet_cdc
    STATIC
        src/gear_hash.cpp
        src/chunker.cpp
        src/hashing_chunker.cpp
        src/file_chunker.cpp
        src/reference_manifest.cpp
        src/chunk_validation.cpp
        src/chunk_hash.cpp
)
add_library(xet_cdc::xet_cdc ALIAS xet_cdc)

target_compile_features(xet_cdc PUBLIC cxx_std_20)
target_include_directories(
    xet_cdc
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_link_libraries(
    xet_cdc
    PRIVATE
        BLAKE3::blake3
        xet_cdc_warnings
        xet_cdc_sanitizers
)
set_target_properties(xet_cdc PROPERTIES CXX_EXTENSIONS OFF)

add_executable(xet-cdc app/main.cpp)
target_link_libraries(
    xet-cdc
    PRIVATE
        xet_cdc::xet_cdc
        xet_cdc_warnings
        xet_cdc_sanitizers
)
set_target_properties(xet-cdc PROPERTIES CXX_EXTENSIONS OFF)

if(XET_CDC_BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()
