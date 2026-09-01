cmake_minimum_required(VERSION 3.21)

project(
    xet_cdc_lab
    VERSION 0.1.0
    DESCRIPTION "A C++20 exploration of Hugging Face Xet content-defined chunking"
    LANGUAGES CXX
)

option(XET_CDC_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)
option(XET_CDC_BUILD_TESTING "Build the xet-cdc-lab tests" ${PROJECT_IS_TOP_LEVEL})
option(XET_CDC_ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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

add_library(xet_cdc INTERFACE)
add_library(xet_cdc::xet_cdc ALIAS xet_cdc)

target_compile_features(xet_cdc INTERFACE cxx_std_20)
target_include_directories(
    xet_cdc
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

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
