cmake_minimum_required(VERSION 3.20)

project(
    xet_cdc_lab
    VERSION 0.1.0
    DESCRIPTION "A C++20 exploration of Hugging Face Xet content-defined chunking"
    LANGUAGES CXX
)

include(CTest)

option(XET_CDC_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

add_library(xet_cdc INTERFACE)
add_library(xet_cdc::xet_cdc ALIAS xet_cdc)

target_compile_features(xet_cdc INTERFACE cxx_std_20)
target_include_directories(
    xet_cdc
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(
        xet_cdc
        INTERFACE
            -Wall
            -Wextra
            -Wpedantic
            $<$<BOOL:${XET_CDC_WARNINGS_AS_ERRORS}>:-Werror>
    )
elseif(MSVC)
    target_compile_options(
        xet_cdc
        INTERFACE
            /W4
            $<$<BOOL:${XET_CDC_WARNINGS_AS_ERRORS}>:/WX>
    )
endif()

add_executable(xet-cdc app/main.cpp)
target_link_libraries(xet-cdc PRIVATE xet_cdc::xet_cdc)

if(BUILD_TESTING)
    add_executable(protocol_constants_test tests/protocol_constants_test.cpp)
    target_link_libraries(protocol_constants_test PRIVATE xet_cdc::xet_cdc)
    add_test(NAME protocol_constants COMMAND protocol_constants_test)
endif()

