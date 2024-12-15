# Copyright (C) 2019 Yaoyuan <ibireme@gmail.com>.
# Released under the MIT License:
# https://github.com/ibireme/yyjson/blob/master/LICENSE
# modified by authors of 0ph1uch1/pyyjson

cmake_minimum_required(VERSION 3.18)
project(pyyjson VERSION 0.9.0 LANGUAGES C CXX)
set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 17)
set(PYYJSON_SOVERSION 0)

# ------------------------------------------------------------------------------
# Search Python Package
find_package(Python3 COMPONENTS Development)

# check for python3ss
if((NOT Python3_INCLUDE_DIRS) OR (NOT Python3_LIBRARIES))
    message(FATAL_ERROR "Python3 not found")
endif()

# ------------------------------------------------------------------------------
# Build Type
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    if(NOT CMAKE_BUILD_TYPE)
        message(STATUS "No build type selected, default to: Release")
        set(CMAKE_BUILD_TYPE Release)
    endif()
endif()

# ------------------------------------------------------------------------------
# Build Options for tests and docs
option(ASAN_ENABLED "Build with asan" OFF)

# option(YYJSON_BUILD_TESTS "Build all tests" OFF)
# option(YYJSON_BUILD_FUZZER "Build fuzzer" OFF)
# option(YYJSON_BUILD_MISC "Build misc" OFF)
# option(YYJSON_BUILD_DOC "Build documentation with doxygen" OFF)
# option(YYJSON_ENABLE_COVERAGE "Enable code coverage for tests" OFF)
# option(YYJSON_ENABLE_VALGRIND "Enable valgrind memory checker for tests" OFF)
# option(YYJSON_ENABLE_SANITIZE "Enable sanitizer for tests" OFF)
# option(YYJSON_ENABLE_FASTMATH "Enable fast-math for tests" OFF)
# option(YYJSON_FORCE_32_BIT "Force 32-bit for tests" OFF)

# ------------------------------------------------------------------------------
# Compilation options, see yyjson.h for more explanation
# option(YYJSON_DISABLE_READER "Disable JSON reader" OFF)
# option(YYJSON_DISABLE_WRITER "Disable JSON writer" OFF)
# option(YYJSON_DISABLE_UTILS "Disable JSON Pointer, JSON Patch, JSON Merge Patch" OFF)
# option(YYJSON_DISABLE_FAST_FP_CONV "Disable custom floating-point number conversion" OFF)
# option(YYJSON_DISABLE_NON_STANDARD "Disable non-standard JSON support" OFF)
# option(YYJSON_DISABLE_UTF8_VALIDATION "Disable UTF-8 validation" OFF)
# option(YYJSON_DISABLE_UNALIGNED_MEMORY_ACCESS "Disable unaligned memory access explicit" OFF)

# if(YYJSON_DISABLE_READER)
# add_definitions(-DYYJSON_DISABLE_READER)
# endif()
# if(YYJSON_DISABLE_WRITER)
# add_definitions(-DYYJSON_DISABLE_WRITER)
# endif()
# if(YYJSON_DISABLE_UTILS)
# add_definitions(-DYYJSON_DISABLE_UTILS)
# endif()
# if(YYJSON_DISABLE_FAST_FP_CONV)
# add_definitions(-DYYJSON_DISABLE_FAST_FP_CONV)
# endif()
# if(YYJSON_DISABLE_NON_STANDARD)
# add_definitions(-DYYJSON_DISABLE_NON_STANDARD)
# endif()
# if(YYJSON_DISABLE_UTF8_VALIDATION)
# add_definitions(-DYYJSON_DISABLE_UTF8_VALIDATION)
# endif()
# if(YYJSON_DISABLE_UNALIGNED_MEMORY_ACCESS)
# add_definitions(-DYYJSON_DISABLE_UNALIGNED_MEMORY_ACCESS)
# endif()
add_definitions(-DYYJSON_DISABLE_NON_STANDARD=1)
add_definitions(-DYYJSON_DISABLE_UTF8_VALIDATION=1)
add_definitions(-DYYJSON_DISABLE_UTILS=1)

# add_definitions(-DYYJSON_DISABLE_WRITER=1)
# add_definitions(-DPYYJSON_USE_XXHASH=1)

# ------------------------------------------------------------------------------
# Library
add_library(pyyjson SHARED
    src/decode/decode.c
    src/pyyjson.c
    src/encode/encode_impl.c
    src/unicode/utils.c
    src/simd/cvt.c
    src/simd/mask_table.c
    src/tls.c
)
target_include_directories(pyyjson PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src> ${Python3_INCLUDE_DIRS})

# set_target_properties(pyyjson PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION ${PYYJSON_SOVERSION})
target_link_libraries(pyyjson PUBLIC ${Python3_LIBRARIES})
set_target_properties(pyyjson PROPERTIES PREFIX "")

# ------------------------------------------------------------------------------
# Project Config
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(XcodeProperty)

if(XCODE)
    set(YYJSON_FLAGS "-Wall -Wextra -Werror -pedantic -pedantic-errors")

    if(YYJSON_ENABLE_FASTMATH)
        set(YYJSON_FLAGS "${YYJSON_FLAGS} -ffast-math")
    endif()

    set_default_xcode_property(pyyjson)
    set_xcode_deployment_version(pyyjson "10.13" "12.0" "12.0" "4.0")

    set_xcode_property(pyyjson GCC_C_LANGUAGE_STANDARD "c89")
    set_xcode_property(pyyjson CLANG_CXX_LANGUAGE_STANDARD "c++98")

    set_xcode_property(pyyjson OTHER_CFLAGS[variant=Debug] ${YYJSON_FLAGS})
    set_xcode_property(pyyjson OTHER_CFLAGS[variant=MinSizeRel] ${YYJSON_FLAGS})
    set_xcode_property(pyyjson OTHER_CFLAGS[variant=RelWithDebInfo] ${YYJSON_FLAGS})
    set_xcode_property(pyyjson OTHER_CFLAGS[variant=Release] ${YYJSON_FLAGS})

elseif(MSVC)
    set(YYJSON_FLAGS "/utf-8")

    if(YYJSON_ENABLE_FASTMATH)
        set(YYJSON_FLAGS "${YYJSON_FLAGS} /fp:fast")
    endif()

    target_compile_options(pyyjson PRIVATE $<$<C_COMPILER_ID:MSVC>:${YYJSON_FLAGS}>)
    target_compile_options(pyyjson PRIVATE $<$<CXX_COMPILER_ID:MSVC>:${YYJSON_FLAGS}>)

elseif(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang|Intel")
    if(CMAKE_C_COMPILER_ID MATCHES Clang)
        # https://github.com/llvm/llvm-project/issues/63963
        set(YYJSON_FLAGS "-Wno-constant-logical-operand")
    else()
        set(YYJSON_FLAGS "")
    endif()

    if(YYJSON_ENABLE_FASTMATH)
        set(YYJSON_FLAGS "-ffast-math")
    endif()

    if(CMAKE_BUILD_TYPE MATCHES "RelWithDebInfo")
        message("Release mode with debug info, enabling maximal optimization and debug symbols")
        target_compile_options(pyyjson PRIVATE -Werror -pedantic -pedantic-errors -O3)
    elseif(CMAKE_BUILD_TYPE MATCHES ".*Rel.*")
        message("Release mode, enabling maximal optimization")
        target_compile_options(pyyjson PRIVATE -Werror -pedantic -pedantic-errors -O3)
    else()
        target_compile_options(pyyjson PRIVATE -O0)

        if(ASAN_ENABLED)
            message("ENABLE ASAN")
            target_link_libraries(pyyjson PRIVATE asan)
            set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address")
            target_compile_definitions(pyyjson PRIVATE PYYJSON_ASAN_CHECK=1)
        endif()

        message("Debug mode, enabling debug symbols")
    endif()

    target_compile_options(pyyjson PRIVATE $<$<COMPILE_LANGUAGE:C>:${YYJSON_FLAGS}>)
    target_compile_options(pyyjson PRIVATE $<$<COMPILE_LANGUAGE:CXX>:${YYJSON_FLAGS}>)
endif()

include(CheckIncludeFile)
check_include_file(stdint.h YYJSON_HAS_STDINT_H)
check_include_file(stdbool.h YYJSON_HAS_STDBOOL_H)

# if(BUILD_SHARED_LIBS)
if(WIN32)
    target_compile_definitions(pyyjson PUBLIC
        $<BUILD_INTERFACE:YYJSON_EXPORTS>
        $<INSTALL_INTERFACE:YYJSON_IMPORTS>)
endif()

# endif()
message("CMAKE_C_COMPILER_ID = ${CMAKE_C_COMPILER_ID}")

# Werror for Release build
# TODO move this to GNU|Clang
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
    target_compile_definitions(pyyjson PRIVATE PYYJSON_DETECT_SIMD=1 TARGET_SIMD_ARCH=${TARGET_SIMD_ARCH})
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
else()
endif()

if(MSVC)
    # TODO remove this
    # target_compile_options(pyyjson PRIVATE /arch:AVX512F /arch:AVX512BW)
    # target_compile_options(pyyjson PRIVATE /arch:AVX2)
    target_compile_options(pyyjson PRIVATE)
else()
    # TODO remove this
    # target_compile_options(pyyjson PRIVATE -march=native)
    # target_compile_options(pyyjson PRIVATE -mavx512f -mavx512bw)
    # target_compile_options(pyyjson PRIVATE -mavx2)
    # target_compile_options(pyyjson PRIVATE -msse4.1 -msse4.2)
    target_compile_options(pyyjson PRIVATE)
endif()
# ------------------------------------------------------------------------------
# Install
if(APPLE)
    set_target_properties(pyyjson PROPERTIES SUFFIX ".so")
endif(APPLE)

install(TARGETS pyyjson LIBRARY DESTINATION .)
