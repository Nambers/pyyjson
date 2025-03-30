// requires: READ

#include "decode.h"
#include "pyyjson.h"
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
 This table is used to convert 4 hex character sequence to a number.
 A valid hex character [0-9A-Fa-f] will mapped to it's raw number [0x00, 0x0F],
 an invalid hex character will mapped to [0xF0].
 (generate with misc/make_tables.c)
 */
extern const u8 hex_conv_table[256];

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
    pyyjson_align(sizeof(_FROM_TYPE) * 4) static const _FROM_TYPE t[4] = {'t', 'r', 'u', 'e'};
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
    pyyjson_align(sizeof(_FROM_TYPE) * 4) static const _FROM_TYPE t[4] = {'a', 'l', 's', 'e'};
    if (likely(end >= cur + 4 && memcmp(cur + 1, t, 4 * sizeof(_FROM_TYPE)) == 0)) {
        *ptr = cur + 5;
        return true;
    }
    return false;
}

/** Read 'null' literal, '*cur' should be 'n'. */
force_inline bool _READ_NULL(const _FROM_TYPE **restrict ptr, const _FROM_TYPE *restrict end) {
    _FROM_TYPE *cur = (_FROM_TYPE *)*ptr;
    pyyjson_align(sizeof(_FROM_TYPE) * 4) static const _FROM_TYPE t[4] = {'n', 'u', 'l', 'l'};
    if (likely(end >= cur + 4 && memcmp(cur, t, 4 * sizeof(_FROM_TYPE)) == 0)) {
        *ptr = cur + 4;
        return true;
    }
    return false;
}

/** Read 'Infinity' literal (ignoring case). */
force_inline bool _READ_INF(const _FROM_TYPE **ptr, const _FROM_TYPE *end) {
#define _READ_INF_SIMD_TYPE PYYJSON_CONCAT4(VECTOR, READ_UNSIGNED_BIT_NAME, READ_BIT_SIZEx8, A)
    if (unlikely(end < *ptr + 8)) {
        return false;
    }
    pyyjson_align(sizeof(_FROM_TYPE) * 8) static const _FROM_TYPE _mask[8] = {
            ~(_FROM_TYPE)0x20, ~(_FROM_TYPE)0x20, ~(_FROM_TYPE)0x20, ~(_FROM_TYPE)0x20,
            ~(_FROM_TYPE)0x20, ~(_FROM_TYPE)0x20, ~(_FROM_TYPE)0x20, ~(_FROM_TYPE)0x20};
    pyyjson_align(sizeof(_FROM_TYPE) * 8) static const _FROM_TYPE _template[8] = {
            'I', 'N', 'F', 'I', 'N', 'I', 'T', 'Y'};
    _READ_INF_SIMD_TYPE data;
    memcpy(&data, *ptr, sizeof(data));
    data = data & *(_READ_INF_SIMD_TYPE *)&_mask;
    if (likely(0 == memcmp(&data, &_template, sizeof(data)))) {
        *ptr += 8;
        return true;
    }
    return false;
#undef _READ_INF_SIMD_TYPE
}

/** Read 'NaN' literal (ignoring case). */
force_inline bool _READ_NAN(const _FROM_TYPE **restrict ptr, const _FROM_TYPE *restrict end) {
#define _READ_NAN_SIMD_TYPE PYYJSON_CONCAT4(VECTOR, READ_UNSIGNED_BIT_NAME, READ_BIT_SIZEx4, A)
    if (end < *ptr + 3) {
        return false;
    }
    // it is safe to load *end, so here we load `4 * sizeof(_FROM_TYPE)` bytes
    pyyjson_align(sizeof(_FROM_TYPE) * 4) static const _FROM_TYPE _mask[4] = {~(_FROM_TYPE)0x20, ~(_FROM_TYPE)0x20, ~(_FROM_TYPE)0x20, 0};
    pyyjson_align(sizeof(_FROM_TYPE) * 4) static const _FROM_TYPE _template[4] = {'N', 'A', 'N', 0};
    _READ_NAN_SIMD_TYPE data;
    memcpy(&data, *ptr, sizeof(data));
    data = data & *(_READ_NAN_SIMD_TYPE *)&_mask;
    if (likely(0 == memcmp(&data, &_template, sizeof(data)))) {
        *ptr += 3;
        return true;
    }
    return false;
#undef _READ_NAN_SIMD_TYPE
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
