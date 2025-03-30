#include "test.h"
#include "simd/simd_detect.h"
#ifdef _WIN32
#    include <windows.h>
#else
#    include <dlfcn.h>
#endif

#if BUILD_MULTI_LIB && PYYJSON_X86
#    if SIMD_BIT_SIZE == 512
#        define GUARDED_SIMD                         \
            do {                                     \
                if (!_SupportAVX512) return SKIPPED; \
            } while (0)
#    elif SIMD_BIT_SIZE == 256
#        define GUARDED_SIMD                       \
            do {                                   \
                if (!_SupportAVX2) return SKIPPED; \
            } while (0)
#    else
#        define GUARDED_SIMD ((void)0)
#    endif
#else
#    define GUARDED_SIMD ((void)0)
#endif

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define U16_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(_u) ((_u) & 0x80 ? '1' : '0'), \
                           ((_u) & 0x40 ? '1' : '0'), \
                           ((_u) & 0x20 ? '1' : '0'), \
                           ((_u) & 0x10 ? '1' : '0'), \
                           ((_u) & 0x08 ? '1' : '0'), \
                           ((_u) & 0x04 ? '1' : '0'), \
                           ((_u) & 0x02 ? '1' : '0'), \
                           ((_u) & 0x01 ? '1' : '0')
#define U16_TO_BINARY(_u) ((_u) & 0x8000 ? '1' : '0'), \
                          ((_u) & 0x4000 ? '1' : '0'), \
                          ((_u) & 0x2000 ? '1' : '0'), \
                          ((_u) & 0x1000 ? '1' : '0'), \
                          ((_u) & 0x800 ? '1' : '0'),  \
                          ((_u) & 0x400 ? '1' : '0'),  \
                          ((_u) & 0x200 ? '1' : '0'),  \
                          ((_u) & 0x100 ? '1' : '0'),  \
                          ((_u) & 0x80 ? '1' : '0'),   \
                          ((_u) & 0x40 ? '1' : '0'),   \
                          ((_u) & 0x20 ? '1' : '0'),   \
                          ((_u) & 0x10 ? '1' : '0'),   \
                          ((_u) & 0x08 ? '1' : '0'),   \
                          ((_u) & 0x04 ? '1' : '0'),   \
                          ((_u) & 0x02 ? '1' : '0'),   \
                          ((_u) & 0x01 ? '1' : '0')

#define TEST_STRINGIZE_EX(_x) #_x
#define TEST_STRINGIZE(_x) TEST_STRINGIZE_EX(_x)

static force_noinline uintptr_t find_extension_symbol(const char *symbol_name) {
#ifdef _WIN32
    static HMODULE handle = NULL;
    if (!handle) handle = GetModuleHandle(NULL);
    if (!handle) return 0;
    uintptr_t ret = (uintptr_t)GetProcAddress(handle, symbol_name);
    return ret;
#else
    static void *handle = NULL;
    if (!handle) handle = dlopen(NULL, RTLD_NOW);
    if (!handle) return 0;
    uintptr_t ret = (uintptr_t)dlsym(handle, symbol_name);
    return ret;
#endif
}

int SIMD_NAME_MODIFIER(test_elevate_1_2_to_128)(void) {
#if PYYJSON_AARCH
    return INVALID;
#else
    GUARDED_SIMD;
    u8 input[16];
    u16 dst[8];
    usize loop_count = 16;
    for (usize _ = 0; _ < loop_count; _++) {
        //
        RANDOM_FILL(input);
        GARBAGE_FILL(dst);
        //
        write_128(dst, elevate_1_2_to_128(load_128(input)));
        for (usize i = 0; i < COUNT_OF(dst); i++) {
            CHECK(dst[i] == input[i]);
        }
    }
    return PASSED;
#endif
}

int SIMD_NAME_MODIFIER(test_elevate_1_4_to_128)(void) {
#if PYYJSON_AARCH
    return INVALID;
#else
    GUARDED_SIMD;
    u8 input[16];
    u32 dst[4];
    usize loop_count = 16;
    for (usize _ = 0; _ < loop_count; _++) {
        //
        RANDOM_FILL(input);
        GARBAGE_FILL(dst);
        //
        write_128(dst, elevate_1_4_to_128(load_128(input)));
        for (usize i = 0; i < COUNT_OF(dst); i++) {
            CHECK(dst[i] == input[i]);
        }
    }
    return PASSED;
#endif
}

int SIMD_NAME_MODIFIER(test_elevate_2_4_to_128)(void) {
#if PYYJSON_AARCH
    return INVALID;
#else
    GUARDED_SIMD;
    u16 input[8];
    u32 dst[4];
    usize loop_count = 16;
    for (usize _ = 0; _ < loop_count; _++) {
        //
        RANDOM_FILL(input);
        GARBAGE_FILL(dst);
        //
        write_128(dst, elevate_2_4_to_128(load_128(input)));
        for (usize i = 0; i < COUNT_OF(dst); i++) {
            CHECK(dst[i] == input[i]);
        }
    }
    return PASSED;
#endif
}

int SIMD_NAME_MODIFIER(test_ucs2_encode_3bytes_utf8)(void) {
#if PYYJSON_AARCH
    return INVALID;
#else
#    if __AVX512F__ && __AVX512BW__
    GUARDED_SIMD;
    u16 input[21];
    u8 output[64];
    for (int i = 0; i < COUNT_OF(input); ++i) {
        input[i] = 0x800 + (rand() % (0x10000 - 0x800));
    }
    ucs2_encode_3bytes_utf8_avx512(input, output);
    for (int i = 0; i < COUNT_OF(input); ++i) {
        u32 uni;
        memcpy(&uni, &output[i * 3], 4);
        u16 rt = ((uni & 0x0f) << 12) | ((uni & 0x3f00) >> 2) | ((uni & 0x3f0000) >> 16);
        CHECK(rt == input[i]);
    }
    return PASSED;
#    elif __AVX2__
    GUARDED_SIMD;
    u16 input[16];
    u8 output[56];
    for (int i = 0; i < COUNT_OF(input); ++i) {
        input[i] = 0x800 + (rand() % (0x10000 - 0x800));
    }
    ucs2_encode_3bytes_utf8_avx2(input, output);
    for (int i = 0; i < COUNT_OF(input); ++i) {
        u32 uni;
        memcpy(&uni, &output[i * 3], 4);
        u16 rt = ((uni & 0x0f) << 12) | ((uni & 0x3f00) >> 2) | ((uni & 0x3f0000) >> 16);
        CHECK(rt == input[i]);
    }
    return PASSED;
#    else
    return INVALID;
#    endif
#endif
}

int SIMD_NAME_MODIFIER(test_long_elevate_1_2)(void) {
    GUARDED_SIMD;
    for (usize _ = 0; _ < 10; _++) {
        static const usize buffer_len = (1 << 11);
        pyyjson_align(64) u8 buffer[buffer_len];
        pyyjson_align(64) u8 buffer_reference[buffer_len];
        GARBAGE_FILL(buffer);
        usize random_u8_start_index = (rand() % buffer_len) & (~(usize)1);
        usize out_u16_length = (usize)rand() % ((buffer_len - random_u8_start_index) / 2);
        // initialize random content
        fill_random_buffer(buffer + random_u8_start_index, out_u16_length);
        // find target func
        uintptr_t _func = find_extension_symbol(TEST_STRINGIZE(SIMD_NAME_MODIFIER(long_back_elevate_1_2)));
        if (!_func) return FAILED;
        typedef void (*TestFuncType)(u16 *, u8 *, Py_ssize_t);
        TestFuncType func = (TestFuncType)_func;
        // backup memory for reference
        memcpy(buffer_reference, buffer, sizeof(buffer));
        // call target func
        func((u16 *)(buffer + random_u8_start_index), buffer + random_u8_start_index, out_u16_length);
        // check garbage content
        u8 *ref_start = buffer_reference + random_u8_start_index;
        u16 *start = (u16 *)(buffer + random_u8_start_index);
        for (usize i = 0; i < random_u8_start_index; ++i) {
            CHECK(buffer[i] == 0xfa);
            CHECK(buffer_reference[i] == 0xfa);
        }
        for (usize i = (random_u8_start_index / 2) + out_u16_length; i < buffer_len / 2; i++) {
            CHECK(((u16 *)buffer)[i] == 0xfafa);
        }
        for (usize i = random_u8_start_index + out_u16_length; i < buffer_len; ++i) {
            CHECK(buffer_reference[i] == 0xfa);
        }
        // check content
        for (usize i = 0; i < out_u16_length; ++i) {
            CHECK(ref_start[i] == start[i]);
        }
    }
    return PASSED;
}
