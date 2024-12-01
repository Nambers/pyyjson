#ifndef PYYJSON_ENCODE_SHARED_H
#define PYYJSON_ENCODE_SHARED_H

#include "encode.h"
// #include "yyjson.h"
#include "pyyjson.h"
#include <stddef.h>


#define _Quote (34)
#define _Slash (92)
#define _MinusOne (-1)
#define ControlMax (32)


/*==============================================================================
 * Types
 *============================================================================*/

enum IndentLevel {
    NONE = 0,
    INDENT_2 = 2,
    INDENT_4 = 4,
};

enum UCSKind {
    UCS1 = 1,
    UCS2 = 2,
    UCS4 = 4,
};

enum X86SIMDLevel {
    SSE2,
    SSE3,
    SSE4,
    AVX,
    AVX2,
    AVX512,
};


#define CONTROL_SEQ_ESCAPE_PREFIX _Slash, 'u', '0', '0'
#define CONTROL_SEQ_ESCAPE_SUFFIX '\0', '\0'
#define CONTROL_SEQ_ESCAPE_MIDDLE CONTROL_SEQ_ESCAPE_SUFFIX, CONTROL_SEQ_ESCAPE_SUFFIX
#define CONTROL_SEQ_ESCAPE_FULL_ZERO CONTROL_SEQ_ESCAPE_MIDDLE, CONTROL_SEQ_ESCAPE_MIDDLE
#define CONTROL_SEQ_ESCAPE_FULL_ZERO_REPEAT2 CONTROL_SEQ_ESCAPE_FULL_ZERO, CONTROL_SEQ_ESCAPE_FULL_ZERO
#define CONTROL_SEQ_ESCAPE_FULL_ZERO_REPEAT4 CONTROL_SEQ_ESCAPE_FULL_ZERO_REPEAT2, CONTROL_SEQ_ESCAPE_FULL_ZERO_REPEAT2
#define CONTROL_SEQ_ESCAPE_FULL_ZERO_REPEAT8 CONTROL_SEQ_ESCAPE_FULL_ZERO_REPEAT4, CONTROL_SEQ_ESCAPE_FULL_ZERO_REPEAT4
#define CONTROL_SEQ_ESCAPE_FULL_ZERO_REPEAT16 CONTROL_SEQ_ESCAPE_FULL_ZERO_REPEAT8, CONTROL_SEQ_ESCAPE_FULL_ZERO_REPEAT8
#define CONTROL_SEQ_ESCAPE_FULL_ZERO_REPEAT32 CONTROL_SEQ_ESCAPE_FULL_ZERO_REPEAT16, CONTROL_SEQ_ESCAPE_FULL_ZERO_REPEAT16

static Py_ssize_t _ControlJump[_Slash + 1] = {
        6, 6, 6, 6, 6, 6, 6, 6, 2, 2, 2, 6, 2, 2, 6, 6, // 0-15
        6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, // 16-31
        0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 32-47
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 48-63
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 64-79
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2           // 80-92
};

// static i32 _ControlLengthAdd[ControlMax] = {
//         5, 5, 5, 5, 5, 5, 5, 5, 1, 1, 1, 5, 1, 1, 5, 5, // 0-15
//         5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, // 16-31
// };

/*==============================================================================
 * Buffer
 *============================================================================*/

static_assert((PYYJSON_ENCODE_DST_BUFFER_INIT_SIZE % 64) == 0, "(PYYJSON_ENCODE_DST_BUFFER_INIT_SIZE % 64) == 0");


/*==============================================================================
 * Utils
 *============================================================================*/

/** Returns whether the size is power of 2 (size should not be 0). */
static_inline bool size_is_pow2(usize size) {
    return (size & (size - 1)) == 0;
}

/** Align size upwards (may overflow). */
force_inline usize size_align_up(usize size, usize align) {
    if (size_is_pow2(align)) {
        return (size + (align - 1)) & ~(align - 1);
    } else {
        return size + align - (size + align - 1) % align - 1;
    }
}

/*
 * Split tail length into multi parts.
 */
force_inline void split_tail_len_two_parts(Py_ssize_t tail_len, Py_ssize_t check_count, Py_ssize_t *restrict part1, Py_ssize_t *restrict part2) {
    assert(tail_len > 0 && tail_len < check_count);
    assert(check_count / 2 * 2 == check_count);
    const Py_ssize_t check_half = check_count / 2;
    Py_ssize_t p1, p2;
    p2 = tail_len > check_half ? check_half : tail_len;
    p1 = tail_len - p2;
    assert(p1 >= 0 && p2 >= 0);
    assert(p1 <= check_half && p2 <= check_half);
    assert(p1 + p2 == tail_len);
    *part2 = p2;
    *part1 = p1;
}

force_inline void split_tail_len_four_parts(Py_ssize_t tail_len, Py_ssize_t check_count, Py_ssize_t *restrict part1, Py_ssize_t *restrict part2, Py_ssize_t *restrict part3, Py_ssize_t *restrict part4) {
    assert(tail_len > 0 && tail_len < check_count);
    assert(check_count / 4 * 4 == check_count);
    const Py_ssize_t orig_tail_len = tail_len;
    const Py_ssize_t check_quad = check_count / 4;
    Py_ssize_t p1, p2, p3, p4;
    p4 = tail_len > check_quad ? check_quad : tail_len;
    tail_len -= p4;
    p3 = tail_len > check_quad ? check_quad : tail_len;
    tail_len -= p3;
    p2 = tail_len > check_quad ? check_quad : tail_len;
    tail_len -= p2;
    p1 = tail_len;
    assert(p1 >= 0 && p2 >= 0 && p3 >= 0 && p4 >= 0);
    assert(p1 <= check_quad && p2 <= check_quad && p3 <= check_quad && p4 <= check_quad);
    assert(p1 + p2 + p3 + p4 == orig_tail_len);
    *part4 = p4;
    *part3 = p3;
    *part2 = p2;
    *part1 = p1;
}

force_inline Py_ssize_t get_indent_char_count(Py_ssize_t cur_nested_depth, Py_ssize_t indent_level) {
    return indent_level ? (indent_level * cur_nested_depth + 1) : 0;
}

/*==============================================================================
 * Python Utils
 *============================================================================*/

force_inline void *get_unicode_data(PyObject *unicode) {
    if (((PyASCIIObject *) unicode)->state.ascii) {
        return (void *) (((PyASCIIObject *) unicode) + 1);
    }
    return (void *) (((PyCompactUnicodeObject *) unicode) + 1);
}

force_inline bool pylong_is_unsigned(PyObject *obj) {
#if PY_MINOR_VERSION >= 12
    return !(bool) (((PyLongObject *) obj)->long_value.lv_tag & 2);
#else
    return ((PyVarObject *) obj)->ob_size > 0;
#endif
}

force_inline bool pylong_is_zero(PyObject *obj) {
#if PY_MINOR_VERSION >= 12
    return (bool) (((PyLongObject *) obj)->long_value.lv_tag & 1);
#else
    return ((PyVarObject *) obj)->ob_size == 0;
#endif
}

// PyErr may occur.
force_inline bool pylong_value_unsigned(PyObject *obj, u64 *value) {
#if PY_MINOR_VERSION >= 12
    if (likely(((PyLongObject *) obj)->long_value.lv_tag < (2 << _PyLong_NON_SIZE_BITS))) {
        *value = (u64) * ((PyLongObject *) obj)->long_value.ob_digit;
        return true;
    }
#endif
    unsigned long long v = PyLong_AsUnsignedLongLong(obj);
    if (unlikely(v == (unsigned long long) -1 && PyErr_Occurred())) {
        return false;
    }
    *value = (u64) v;
    static_assert(sizeof(unsigned long long) <= sizeof(u64), "sizeof(unsigned long long) <= sizeof(u64)");
    return true;
}

force_inline i64 pylong_value_signed(PyObject *obj, i64 *value) {
#if PY_MINOR_VERSION >= 12
    if (likely(((PyLongObject *) obj)->long_value.lv_tag < (2 << _PyLong_NON_SIZE_BITS))) {
        i64 sign = 1 - (i64) ((((PyLongObject *) obj)->long_value.lv_tag & 3));
        *value = sign * (i64) * ((PyLongObject *) obj)->long_value.ob_digit;
        return true;
    }
#endif
    long long v = PyLong_AsLongLong(obj);
    if (unlikely(v == -1 && PyErr_Occurred())) {
        return false;
    }
    *value = (i64) v;
    static_assert(sizeof(long long) <= sizeof(i64), "sizeof(long long) <= sizeof(i64)");
    return true;
}

force_inline int pydict_next(PyObject *op, Py_ssize_t *ppos, PyObject **pkey,
                             PyObject **pvalue) {
#if PY_MINOR_VERSION >= 13
    return PyDict_Next(op, ppos, pkey, pvalue);
#else
    return _PyDict_Next(op, ppos, pkey, pvalue, NULL);
#endif
}

/*==============================================================================
 * Constants
 *============================================================================*/

pyyjson_align(64) const i8 _Quote_i8[64] = {REPEAT_64(_Quote)};
pyyjson_align(64) const i16 _Quote_i16[32] = {REPEAT_32(_Quote)};
pyyjson_align(64) const i32 _Quote_i32[16] = {REPEAT_16(_Quote)};
pyyjson_align(64) const i8 _Slash_i8[64] = {REPEAT_64(_Slash)};
pyyjson_align(64) const i16 _Slash_i16[32] = {REPEAT_32(_Slash)};
pyyjson_align(64) const i32 _Slash_i32[16] = {REPEAT_16(_Slash)};
pyyjson_align(64) const i8 _MinusOne_i8[64] = {REPEAT_64(_MinusOne)};
pyyjson_align(64) const i16 _MinusOne_i16[32] = {REPEAT_32(_MinusOne)};
pyyjson_align(64) const i32 _MinusOne_i32[16] = {REPEAT_16(_MinusOne)};
pyyjson_align(64) const i8 _ControlMax_i8[64] = {REPEAT_64(ControlMax)};
pyyjson_align(64) const i16 _ControlMax_i16[32] = {REPEAT_32(ControlMax)};
pyyjson_align(64) const i32 _ControlMax_i32[16] = {REPEAT_16(ControlMax)};
pyyjson_align(64) const i8 _Seven_i8[64] = {REPEAT_64(7)};
pyyjson_align(64) const i16 _Seven_i16[32] = {REPEAT_32(7)};
pyyjson_align(64) const i32 _Seven_i32[16] = {REPEAT_16(7)};
pyyjson_align(64) const i8 _Eleven_i8[64] = {REPEAT_64(11)};
pyyjson_align(64) const i16 _Eleven_i16[32] = {REPEAT_32(11)};
pyyjson_align(64) const i32 _Eleven_i32[16] = {REPEAT_16(11)};
pyyjson_align(64) const i8 _Fourteen_i8[64] = {REPEAT_64(14)};
pyyjson_align(64) const i16 _Fourteen_i16[32] = {REPEAT_32(14)};
pyyjson_align(64) const i32 _Fourteen_i32[16] = {REPEAT_16(14)};
pyyjson_align(64) const i32 _All_0XFF[16] = {REPEAT_16(-1)};

#endif // PYYJSON_ENCODE_SHARED_H
