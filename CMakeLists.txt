cmake_minimum_required(VERSION 3.18)
project(pyyjson VERSION 0.9.0 LANGUAGES C)
set(CMAKE_C_STANDARD 17)
set(PYYJSON_SOVERSION 0)

# ------------------------------------------------------------------------------
# Build Options for tests and docs
option(ASAN_ENABLED "Build with asan" OFF)

# TODO
option(BUILD_BENCHMARK "Build benchmarks" ON)
option(BUILD_TEST "Build tests" ON)
option(BUILD_SHIPPING_SIMD "Build all simd for shipping" ON)
option(BUILD_AVX512 "Force build avx512, non-shipping" OFF)
option(BUILD_AVX2 "Force build avx2, non-shipping" OFF)
option(BUILD_SSE4 "Force build sse4, non-shipping" OFF)
option(BUILD_NATIVE "Force build with -march=native, non-shipping" OFF)
option(RYU_HUMAN_READABLE "Not use scientific notation output in ryu" OFF)

if(BUILD_SHIPPING_SIMD)
    set(BUILD_MULTI_LIB 1)
else()
    set(BUILD_MULTI_LIB 0)
endif()

# ------------------------------------------------------------------------------
# Search Python Package
if(BUILD_TEST)
    find_package(Python3 COMPONENTS Development REQUIRED)
else()
    find_package(Python3 COMPONENTS Development)
endif()

# check for python3
if((NOT Python3_INCLUDE_DIRS) OR(NOT Python3_LIBRARIES))
    message(FATAL_ERROR "Python3 not found")
endif()

message("Python3_INCLUDE_DIRS = ${Python3_INCLUDE_DIRS}")
message("Python3_LIBRARIES = ${Python3_LIBRARIES}")
message("Python3_LIBRARY_DIRS = ${Python3_LIBRARY_DIRS}")

# ------------------------------------------------------------------------------
# Build Type
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    if(NOT CMAKE_BUILD_TYPE)
        message(STATUS "No build type selected, default to: Release")
        set(CMAKE_BUILD_TYPE Release)
    endif()
endif()

# ------------------------------------------------------------------------------
# Global settings
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# ------------------------------------------------------------------------------
# Libraries

# common definitions
add_library(commonBuild INTERFACE)
target_link_libraries(commonBuild INTERFACE ${Python3_LIBRARIES})
target_include_directories(commonBuild INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src> ${Python3_INCLUDE_DIRS})
target_compile_definitions(commonBuild INTERFACE BUILD_MULTI_LIB=${BUILD_MULTI_LIB})

message("CMAKE_C_COMPILER_ID = ${CMAKE_C_COMPILER_ID}")
message("CMAKE_BUILD_TYPE = ${CMAKE_BUILD_TYPE}")
message("CMAKE_SYSTEM_PROCESSOR = ${CMAKE_SYSTEM_PROCESSOR}")

if(CMAKE_SYSTEM_PROCESSOR MATCHES "(^aarch64)|(^arm64)|(^ARM64)")
    set(PYYJSON_DETECT_SIMD 1)
    set(TARGET_SIMD_ARCH aarch)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^arm")
    set(PYYJSON_DETECT_SIMD 1)
    set(TARGET_SIMD_ARCH aarch)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "(x86)|(x86_64)|(AMD64|amd64)|(^i.86$)")
    set(PYYJSON_DETECT_SIMD 1)
    set(TARGET_SIMD_ARCH x86)
else()
    set(PYYJSON_DETECT_SIMD 0)
endif()

if(PYYJSON_DETECT_SIMD)
    target_compile_definitions(commonBuild INTERFACE PYYJSON_DETECT_SIMD=1 TARGET_SIMD_ARCH=${TARGET_SIMD_ARCH})
else()
    # TODO cannot detect simd
endif()

if(BUILD_BENCHMARK)
    message("ENABLE BENCHMARK")
    target_compile_definitions(commonBuild INTERFACE PYYJSON_BUILD_BENCHMARK=1)
endif()

if(RYU_HUMAN_READABLE)
    target_compile_definitions(commonBuild INTERFACE RYU_HUMAN_READABLE=1)
endif()

if(ASAN_ENABLED)
    message("ENABLE ASAN")
    target_compile_definitions(commonBuild INTERFACE PYYJSON_ASAN_CHECK=1)

    if(MSVC)
        target_compile_options(commonBuild INTERFACE /fsanitize=address)
        target_link_options(commonBuild INTERFACE /fsanitize=address)
    else()
        target_compile_options(commonBuild INTERFACE -fsanitize=address)
        target_link_options(commonBuild INTERFACE -fsanitize=address)
    endif()
endif()

# ------------------------------------------------------------------------------
# setting up pyyjson

# set pyyjson src files
set(SRC_FILES
    src/pyyjson.c
    src/unicode/utils.c
    src/simd/mask_table.c
    src/tls.c
    src/float/tables.c
    src/ryu/d2s.c
    src/decode/utils.c
)

set(SRC_WITH_SIMD
    src/decode/decode.c
    src/encode/encode_impl.c
    src/simd/cvt.c
)

set(SRC_TEST
    src/ctests/main.c
)

set(SRC_TEST_WITH_SIMD
    src/ctests/test.c
)

set(SRC_FUZZER
    src/ctests/fuzzer.c
)

if(BUILD_BENCHMARK)
    set(SRC_FILES ${SRC_FILES} src/benchmark.c)
endif()

# add library pyyjson
if(BUILD_SHIPPING_SIMD)
    if("${TARGET_SIMD_ARCH}" STREQUAL "x86")
        add_library(pyyjson_common OBJECT ${SRC_FILES})
        target_link_libraries(pyyjson_common PUBLIC commonBuild)
        add_library(pyyjson_avx512 OBJECT ${SRC_WITH_SIMD})
        target_link_libraries(pyyjson_avx512 PUBLIC pyyjson_common)
        add_library(pyyjson_avx2 OBJECT ${SRC_WITH_SIMD})
        target_link_libraries(pyyjson_avx2 PUBLIC pyyjson_common)

        # add_library(pyyjson_sse4 OBJECT ${SRC_WITH_SIMD})
        # target_link_libraries(pyyjson_sse4 PUBLIC pyyjson_common)
        add_library(pyyjson_sse2 OBJECT ${SRC_WITH_SIMD})
        target_link_libraries(pyyjson_sse2 PUBLIC pyyjson_common)
        add_library(pyyjson SHARED $<TARGET_OBJECTS:pyyjson_avx512> $<TARGET_OBJECTS:pyyjson_avx2> $<TARGET_OBJECTS:pyyjson_sse2> $<TARGET_OBJECTS:pyyjson_common>)
        target_link_libraries(pyyjson PUBLIC ${Python3_LIBRARIES})

        if(MSVC)
            # MSVC does not support precise SIMD feature settings like /arch:SSE4.2, /arch:AVX512F
            target_compile_options(pyyjson_avx512 PRIVATE /arch:AVX512)
            target_compile_options(pyyjson_avx2 PRIVATE /arch:AVX2)

        else()
            target_compile_options(pyyjson_avx512 PRIVATE -mavx512f -mavx512bw)
            target_compile_options(pyyjson_avx2 PRIVATE -mavx2)

            # target_compile_options(pyyjson_sse4 PRIVATE -msse4.1 -msse4.2)
        endif()
    else()
        message(FATAL_ERROR "TARGET_SIMD_ARCH=${TARGET_SIMD_ARCH} not supported")

        # TODO
    endif()
else()
    add_library(pyyjson SHARED ${SRC_FILES} ${SRC_WITH_SIMD})
    target_link_libraries(pyyjson PRIVATE commonBuild)
endif()

set_target_properties(pyyjson PROPERTIES PREFIX "")

# set_target_properties(pyyjson PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION ${PYYJSON_SOVERSION})

# ------------------------------------------------------------------------------
# Project Config
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(XcodeProperty)

if(XCODE)
    set(PYYJSON_FLAGS)
    list(APPEND PYYJSON_FLAGS "-Wall" "-Wextra" "-Werror" "-pedantic" "-pedantic-errors")

    set_default_xcode_property(commonBuild)
    set_xcode_deployment_version(commonBuild "10.13" "12.0" "12.0" "4.0")

    set_xcode_property(commonBuild GCC_C_LANGUAGE_STANDARD "c17")
    set_xcode_property(commonBuild CLANG_C_LANGUAGE_STANDARD "c17")

    # set_xcode_property(commonBuild CLANG_CXX_LANGUAGE_STANDARD "c++98")
    set_xcode_property(commonBuild OTHER_CFLAGS[variant=Debug] ${PYYJSON_FLAGS})
    set_xcode_property(commonBuild OTHER_CFLAGS[variant=MinSizeRel] ${PYYJSON_FLAGS})
    set_xcode_property(commonBuild OTHER_CFLAGS[variant=RelWithDebInfo] ${PYYJSON_FLAGS})
    set_xcode_property(commonBuild OTHER_CFLAGS[variant=Release] ${PYYJSON_FLAGS})

elseif(MSVC)
    set(PYYJSON_FLAGS)
    list(APPEND PYYJSON_FLAGS "/utf-8")

    target_compile_options(commonBuild INTERFACE $<$<C_COMPILER_ID:MSVC>:${PYYJSON_FLAGS}>)
    target_compile_options(commonBuild INTERFACE $<$<CXX_COMPILER_ID:MSVC>:${PYYJSON_FLAGS}>)

elseif(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang|Intel")
    set(PYYJSON_FLAGS)

    if(CMAKE_C_COMPILER_ID MATCHES Clang)
        # https://github.com/llvm/llvm-project/issues/63963
        list(APPEND PYYJSON_FLAGS "-Wno-constant-logical-operand")
    endif()

    if(CMAKE_BUILD_TYPE MATCHES "RelWithDebInfo")
        message("Release mode with debug info, enabling maximal optimization and debug symbols")
        target_compile_options(commonBuild INTERFACE -Werror -pedantic -pedantic-errors)
    elseif(CMAKE_BUILD_TYPE MATCHES ".*Rel.*")
        message("Release mode, enabling maximal optimization")
        target_compile_options(commonBuild INTERFACE -Werror -pedantic -pedantic-errors)
    else()
        target_compile_options(commonBuild INTERFACE -O0)

        message("Debug mode, enabling debug symbols")
    endif()

    target_compile_options(commonBuild INTERFACE $<$<COMPILE_LANGUAGE:C>:${PYYJSON_FLAGS}>)

    # target_compile_options(commonBuild INTERFACE $<$<COMPILE_LANGUAGE:CXX>:${PYYJSON_FLAGS}>)
endif()

include(CheckIncludeFile)
check_include_file(stdint.h YYJSON_HAS_STDINT_H)
check_include_file(stdbool.h YYJSON_HAS_STDBOOL_H)

if(WIN32)
    target_compile_definitions(commonBuild INTERFACE
        $<BUILD_INTERFACE:YYJSON_EXPORTS>
        $<INSTALL_INTERFACE:YYJSON_IMPORTS>)
endif()

if(MSVC)
    # TODO remove this
    # target_compile_options(commonBuild INTERFACE /arch:AVX512F /arch:AVX512BW)
    # target_compile_options(commonBuild INTERFACE /arch:AVX2)
    target_compile_options(commonBuild INTERFACE)
else()
    # TODO remove this
    # target_compile_options(commonBuild INTERFACE -march=native)
    # target_compile_options(commonBuild INTERFACE -mavx512f -mavx512bw)
    # target_compile_options(commonBuild INTERFACE -mavx2)

    # target_compile_options(commonBuild INTERFACE -msse4.1 -msse4.2)
    target_compile_options(commonBuild INTERFACE)
endif()

# ------------------------------------------------------------------------------
# Install
if(APPLE)
    set_target_properties(pyyjson PROPERTIES SUFFIX ".so")
endif(APPLE)

install(TARGETS pyyjson LIBRARY DESTINATION .)

if(BUILD_TEST)
    include(CTest)

    if(BUILD_SHIPPING_SIMD)
        add_library(pyyjson_test_sse2 OBJECT ${SRC_TEST_WITH_SIMD})
        target_link_libraries(pyyjson_test_sse2 PUBLIC commonBuild)
        add_library(pyyjson_test_avx2 OBJECT ${SRC_TEST_WITH_SIMD})
        target_link_libraries(pyyjson_test_avx2 PUBLIC commonBuild)
        add_library(pyyjson_test_avx512 OBJECT ${SRC_TEST_WITH_SIMD})
        target_link_libraries(pyyjson_test_avx512 PUBLIC commonBuild)

        if(MSVC)
            # MSVC does not support precise SIMD feature settings like /arch:SSE4.2, /arch:AVX512F
            target_compile_options(pyyjson_test_avx512 PRIVATE /arch:AVX512)
            target_compile_options(pyyjson_test_avx2 PRIVATE /arch:AVX2)
        else()
            target_compile_options(pyyjson_test_avx512 PRIVATE -mavx512f -mavx512bw)
            target_compile_options(pyyjson_test_avx2 PRIVATE -mavx2)
        endif()

        add_executable(pyyjson_test ${SRC_TEST} $<TARGET_OBJECTS:pyyjson_test_avx512> $<TARGET_OBJECTS:pyyjson_test_avx2> $<TARGET_OBJECTS:pyyjson_test_sse2>)
    else()
        add_executable(pyyjson_test ${SRC_TEST} ${SRC_TEST_WITH_SIMD})
    endif()

    target_link_libraries(pyyjson_test PUBLIC commonBuild)
    target_link_libraries(pyyjson_test PUBLIC Python3::Python)
    install(TARGETS pyyjson_test RUNTIME DESTINATION .)

    if(CMAKE_C_COMPILER_ID MATCHES Clang)
        file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/test_data/fuzzer/fuzzer.dict" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
        file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/test_data/fuzzer/pyyjson_fuzz.py" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
        file(GLOB TEST_DATA_FILES "${CMAKE_CURRENT_SOURCE_DIR}/test_data/json/**/*.json")
        file(COPY ${TEST_DATA_FILES} DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/corpus")
        add_executable(pyyjson_fuzzer ${SRC_FUZZER})
        target_link_libraries(pyyjson_fuzzer PUBLIC commonBuild)
        target_link_libraries(pyyjson_fuzzer PUBLIC Python3::Python)
        target_compile_options(pyyjson_fuzzer PRIVATE -fsanitize=fuzzer -g -O1)
        target_link_options(pyyjson_fuzzer PRIVATE -fsanitize=fuzzer)
        add_test(pyyjson_fuzzer ${CMAKE_CURRENT_BINARY_DIR}/pyyjson_fuzzer -dict=fuzzer.dict -max_total_time=300 ${CMAKE_CURRENT_BINARY_DIR}/corpus)
    endif()
endif()
