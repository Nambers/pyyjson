#include "test.h"

bool SIMD_NAME_MODIFIER(test_elevate_1_2_to_128)(void) {
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
    return true;
}

bool SIMD_NAME_MODIFIER(test_elevate_1_4_to_128)(void) {
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
    return true;
}

bool SIMD_NAME_MODIFIER(test_elevate_2_4_to_128)(void) {
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
    return true;
}
