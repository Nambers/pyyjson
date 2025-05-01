#include "test.h"
#include "simd/simd_detect.h"
#include "tools.h"

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
#    if __AVX512F__ && __AVX512BW__ && __AVX512VL__
    GUARDED_SIMD;
    u16 input[32];
    u8 output[96];
    for (int i = 0; i < COUNT_OF(input); ++i) {
        input[i] = get_random_3bytes_u16();
    }
    ucs2_encode_3bytes_utf8_avx512((VECTOR_U16_512_A) * (VECTOR_U16_512_U *)input, output);
    return check_ucs2_3bytes(input, output, COUNT_OF(input));
#    elif __AVX2__
    GUARDED_SIMD;
    u16 input[16];
    u8 output[48];
    for (int i = 0; i < COUNT_OF(input); ++i) {
        input[i] = get_random_3bytes_u16();
    }
    ucs2_encode_3bytes_utf8_avx2((VECTOR_U8_256_A) * (VECTOR_U8_256_U *)input, output);
    return check_ucs2_3bytes(input, output, COUNT_OF(input));
#    else
    return INVALID;
#    endif
#endif
}

int SIMD_NAME_MODIFIER(test_ucs2_encode_2bytes_utf8)(void) {
#if PYYJSON_AARCH
    return INVALID;
#else
#    if __AVX512F__ && __AVX512BW__ && __AVX512VL__
    GUARDED_SIMD;
    u16 input[32];
    u8 output[64];
    for (int i = 0; i < COUNT_OF(input); ++i) {
        input[i] = get_random_2bytes_u16();
    }
    ucs2_encode_2bytes_utf8_avx512((VECTOR_U16_512_A) * (VECTOR_U16_512_U *)input, output);
    return check_ucs2_2bytes(input, output, COUNT_OF(input));
#    elif __AVX2__
    GUARDED_SIMD;
    u16 input[16];
    u8 output[32];
    for (int i = 0; i < COUNT_OF(input); ++i) {
        input[i] = get_random_2bytes_u16();
    }
    ucs2_encode_2bytes_utf8_avx2((VECTOR_U8_256_A) * (VECTOR_U8_256_U *)input, output);
    return check_ucs2_2bytes(input, output, COUNT_OF(input));
#    else
    u16 input[8];
    u8 output[16];
    for (int i = 0; i < COUNT_OF(input); ++i) {
        input[i] = get_random_2bytes_u16();
    }
    ucs2_encode_2bytes_utf8_sse2((VECTOR_U8_128_A) * (VECTOR_U8_128_U *)input, output);
    return check_ucs2_2bytes(input, output, COUNT_OF(input));
#    endif
#endif
}

int SIMD_NAME_MODIFIER(test_ucs4_encode_3bytes_utf8)(void) {
#if PYYJSON_AARCH
    return INVALID;
#else
#    if __AVX512F__ && __AVX512BW__ && __AVX512VL__
    GUARDED_SIMD;
    u32 input[16];
    u8 output[48];
    for (int i = 0; i < COUNT_OF(input); ++i) {
        input[i] = get_random_3bytes_u16();
    }
    ucs4_encode_3bytes_utf8_avx512((VECTOR_U32_512_A) * (VECTOR_U32_512_U *)input, output);
    return check_ucs4_3bytes(input, output, COUNT_OF(input));
#    elif __AVX2__
    GUARDED_SIMD;
    u32 input[8];
    u8 output[24];
    for (int i = 0; i < COUNT_OF(input); ++i) {
        input[i] = get_random_3bytes_u16();
    }
    ucs4_encode_3bytes_utf8_avx2((VECTOR_U8_256_A) * (VECTOR_U8_256_U *)input, output);
    return check_ucs4_3bytes(input, output, COUNT_OF(input));
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
