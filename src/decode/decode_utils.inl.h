// requires: READ

#include "decode.h"
#include "simd/simd_impl.h"

#define DECODE_SRC_INFO PYYJSON_CONCAT2(DecodeSrcInfo, COMPILE_READ_UCS_LEVEL)
#define READ_TO_HEX_U16 PYYJSON_CONCAT3(read, READ_BIT_SIZE, to_hex_u16)
#define _READ_TRUE PYYJSON_CONCAT2(_read_true, COMPILE_READ_UCS_LEVEL)
#define _READ_FALSE PYYJSON_CONCAT2(_read_false, COMPILE_READ_UCS_LEVEL)
#define _READ_NULL PYYJSON_CONCAT2(_read_null, COMPILE_READ_UCS_LEVEL)
#define _READ_INF PYYJSON_CONCAT2(_read_inf, COMPILE_READ_UCS_LEVEL)
#define _READ_NAN PYYJSON_CONCAT2(_read_nan, COMPILE_READ_UCS_LEVEL)
#define READ_INF_OR_NAN PYYJSON_CONCAT2(read_inf_or_nan, COMPILE_READ_UCS_LEVEL)

typedef struct DECODE_SRC_INFO {
    const _FROM_TYPE *src;
    const _FROM_TYPE *const src_start;
    const _FROM_TYPE *const src_end;
} DECODE_SRC_INFO;

/**
 Scans an escaped character sequence as a UTF-16 code unit (branchless).
 e.g. "\\u005C" should pass "005C" as `cur`.
 
 This requires the string has 4-byte zero padding.
 */
force_inline bool READ_TO_HEX_U16(const _FROM_TYPE *cur, u16 *val) {
    u16 c0, c1, c2, c3, t0, t1;
    assert(cur[0] <= U8MAX);
    assert(cur[1] <= U8MAX);
    assert(cur[2] <= U8MAX);
    assert(cur[3] <= U8MAX);
    c0 = hex_conv_table[cur[0]];
    c1 = hex_conv_table[cur[1]];
    c2 = hex_conv_table[cur[2]];
    c3 = hex_conv_table[cur[3]];
    t0 = (u16)((c0 << 8) | c2);
    t1 = (u16)((c1 << 8) | c3);
    *val = (u16)((t0 << 4) | t1);
    return ((t0 | t1) & (u16)0xF0F0) == 0;
}

/** Read 'true' literal, '*cur' should be 't'. */
force_inline bool _READ_TRUE(const _FROM_TYPE **restrict ptr, const _FROM_TYPE *restrict end) {
    _FROM_TYPE *cur = (_FROM_TYPE *)*ptr;
    _FROM_TYPE t[4] = {'t', 'r', 'u', 'e'};
    if (likely(end >= cur + 4 && memcmp(cur, t, 4 * sizeof(_FROM_TYPE)) == 0)) {
        *ptr = cur + 4;
        return true;
    }
    return false;
}

/** Read 'false' literal, '*cur' should be 'f'. */
force_inline bool _READ_FALSE(const _FROM_TYPE **restrict ptr, const _FROM_TYPE *restrict end) {
    // the first 'f' is already checked
    _FROM_TYPE *cur = (_FROM_TYPE *)*ptr;
    _FROM_TYPE t[4] = {'a', 'l', 's', 'e'};
    if (likely(end >= cur + 4 && memcmp(cur + 1, t, 4 * sizeof(_FROM_TYPE)) == 0)) {
        *ptr = cur + 5;
        return true;
    }
    return false;
}

/** Read 'null' literal, '*cur' should be 'n'. */
force_inline bool _READ_NULL(const _FROM_TYPE **restrict ptr, const _FROM_TYPE *restrict end) {
    _FROM_TYPE *cur = (_FROM_TYPE *)*ptr;
    _FROM_TYPE t[4] = {'n', 'u', 'l', 'l'};
    if (likely(end >= cur + 4 && memcmp(cur, t, 4 * sizeof(_FROM_TYPE)) == 0)) {
        *ptr = cur + 4;
        return true;
    }
    return false;
}

/** Read 'Infinity' literal (ignoring case). */
force_inline bool _READ_INF(const _FROM_TYPE **ptr, const _FROM_TYPE *end) {
#define COMP_TWICE (COMPILE_READ_UCS_LEVEL == 4 && SIMD_BIT_SIZE < 256)
    if (end > *ptr + 8) {
        return false;
    }
    static const _FROM_TYPE _mask[16] = {
            ~(_FROM_TYPE)0x20, ~(_FROM_TYPE)0x20, ~(_FROM_TYPE)0x20, ~(_FROM_TYPE)0x20,
            ~(_FROM_TYPE)0x20, ~(_FROM_TYPE)0x20, ~(_FROM_TYPE)0x20, ~(_FROM_TYPE)0x20, 0};
    static const _FROM_TYPE template[16] = {'I', 'N', 'F', 'I', 'N', 'I', 'T', 'Y', 0};
#if COMP_TWICE
    SIMD_128 slide1 = load_128((void *)*ptr);
    SIMD_128 slide2 = load_128((void *)((*ptr) + 4));
    SIMD_128 mask1 = load_128_aligned(_mask);
    SIMD_128 mask2 = load_128_aligned(_mask + 4);
    slide1 = simd_and_128(slide1, mask1);
    slide2 = simd_and_128(slide2, mask2);
    if (likely(0 == memcmp(&slide1, &template, 4 * sizeof(u32)) && 0 == memcmp(&slide2, &template[4], 4 * sizeof(u32)))) {
        ptr += 8;
        return true;
    }
    return false;
#else // !COMP_TWICE
#    if COMPILE_READ_UCS_LEVEL == 4
    SIMD_256 slide = load_256((void *)*ptr);
    SIMD_256 mask = load_256_aligned(_mask);
    slide = simd_and_256(slide, mask);
#    elif COMPILE_READ_UCS_LEVEL == 2
    SIMD_128 slide = load_128((void *)*ptr);
    SIMD_128 mask = load_128_aligned(_mask);
    slide = simd_and_128(slide, mask);
#    else
    u64 slide = *(u64 *)*ptr;
    u64 mask = *(u64 *)_mask;
    slide = slide & mask;
#    endif
    // use memcmp and compiler optimization to avoid repeating the same code
    if (likely(0 == memcmp(&slide, &template, sizeof(slide)))) {
        ptr += 8;
        return true;
    }
    return false;
#endif // !COMP_TWICE
#undef COMP_TWICE
}

/** Read 'NaN' literal (ignoring case). */
force_inline bool _READ_NAN(const _FROM_TYPE **restrict ptr, const _FROM_TYPE *restrict end) {
    if (end > *ptr + 3) {
        return false;
    }
    // it is safe to load *end, so here we load `4 * sizeof(_FROM_TYPE)` bytes
    static const _FROM_TYPE _mask[4] = {~(_FROM_TYPE)0x20, ~(_FROM_TYPE)0x20, ~(_FROM_TYPE)0x20, 0};
    static const _FROM_TYPE template[4] = {'N', 'A', 'N', 0};
#if COMPILE_READ_UCS_LEVEL == 4
    SIMD_128 slide = load_128((void *)*ptr);
    SIMD_128 mask = load_128_aligned(_mask);
    slide = simd_and_128(slide, mask);
#elif COMPILE_READ_UCS_LEVEL == 2
    u64 slide = *(u64 *)*ptr;
    u64 mask = *(u64 *)_mask;
    slide = slide & mask;
#else
    u32 slide = *(u32 *)*ptr;
    u32 mask = *(u32 *)_mask;
    slide = slide & mask;
#endif
    // use memcmp and compiler optimization to avoid repeating the same code
    if (likely(0 == memcmp(&slide, &template, sizeof(slide)))) {
        ptr += 3;
        return true;
    }

    return false;
}

/** Read 'Infinity' or 'NaN' literal (ignoring case). */
force_inline PyObject *READ_INF_OR_NAN(bool sign, const _FROM_TYPE **ptr, const _FROM_TYPE *end) {
    if (_READ_INF(ptr, end)) {
        return PyFloat_FromDouble(sign ? -fabs(Py_HUGE_VAL) : fabs(Py_HUGE_VAL));
    }
    if (_READ_NAN(ptr, end)) {
        return PyFloat_FromDouble(sign ? -fabs(Py_NAN) : fabs(Py_NAN));
    }
    return NULL;
}

#undef READ_INF_OR_NAN
#undef _READ_NAN
#undef _READ_INF
#undef _READ_NULL
#undef _READ_FALSE
#undef _READ_TRUE
#undef READ_TO_HEX_U16
#undef DECODE_SRC_INFO
