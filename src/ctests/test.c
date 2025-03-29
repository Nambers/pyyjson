#include "test.h"
#include "simd/simd_detect.h"
#if BUILD_MULTI_LIB
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

int SIMD_NAME_MODIFIER(test_elevate_1_2_to_128)(void) {
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
}

int SIMD_NAME_MODIFIER(test_elevate_1_4_to_128)(void) {
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
}

int SIMD_NAME_MODIFIER(test_elevate_2_4_to_128)(void) {
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
}

int SIMD_NAME_MODIFIER(test_ucs2_encode_3bytes_utf8)(void) {
#if __AVX512F__ && __AVX512BW__
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
#elif __AVX2__
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
#else
    return INVALID;
#endif
}
