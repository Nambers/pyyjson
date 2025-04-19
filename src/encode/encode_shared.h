#ifndef PYYJSON_ENCODE_SHARED_H
#define PYYJSON_ENCODE_SHARED_H

#include "pyyjson.h"
#include <stddef.h>


#define CONTROL_SEQ_ESCAPE_PREFIX _Slash, 'u', '0', '0'
#define CONTROL_SEQ_ESCAPE_SUFFIX '\0', '\0'
#define CONTROL_SEQ_ESCAPE_MIDDLE CONTROL_SEQ_ESCAPE_SUFFIX, CONTROL_SEQ_ESCAPE_SUFFIX
#define CONTROL_SEQ_ESCAPE_FULL_ZERO CONTROL_SEQ_ESCAPE_MIDDLE, CONTROL_SEQ_ESCAPE_MIDDLE
#define CONTROL_SEQ_ESCAPE_FULL_ZERO_REPEAT2 CONTROL_SEQ_ESCAPE_FULL_ZERO, CONTROL_SEQ_ESCAPE_FULL_ZERO
#define CONTROL_SEQ_ESCAPE_FULL_ZERO_REPEAT4 CONTROL_SEQ_ESCAPE_FULL_ZERO_REPEAT2, CONTROL_SEQ_ESCAPE_FULL_ZERO_REPEAT2
#define CONTROL_SEQ_ESCAPE_FULL_ZERO_REPEAT8 CONTROL_SEQ_ESCAPE_FULL_ZERO_REPEAT4, CONTROL_SEQ_ESCAPE_FULL_ZERO_REPEAT4
#define CONTROL_SEQ_ESCAPE_FULL_ZERO_REPEAT16 CONTROL_SEQ_ESCAPE_FULL_ZERO_REPEAT8, CONTROL_SEQ_ESCAPE_FULL_ZERO_REPEAT8
#define CONTROL_SEQ_ESCAPE_FULL_ZERO_REPEAT32 CONTROL_SEQ_ESCAPE_FULL_ZERO_REPEAT16, CONTROL_SEQ_ESCAPE_FULL_ZERO_REPEAT16

// /** Minimum decimal exponent in pow10_sig_table. */
// #define POW10_SIG_TABLE_MIN_EXP -343

// /** Maximum decimal exponent in pow10_sig_table. */
// #define POW10_SIG_TABLE_MAX_EXP 324

// /** Minimum exact decimal exponent in pow10_sig_table */
// #define POW10_SIG_TABLE_MIN_EXACT_EXP 0

// /** Maximum exact decimal exponent in pow10_sig_table */
// #define POW10_SIG_TABLE_MAX_EXACT_EXP 55

/* double number exponent bias */
#define F64_EXP_BIAS 1023

/* double number significand part bits */
#define F64_SIG_BITS 52

/* double number bits */
#define F64_BITS 64

/* double number exponent part bits */
#define F64_EXP_BITS 11

/* double number significand bit mask */
#define F64_SIG_MASK U64(0x000FFFFF, 0xFFFFFFFF)

/* double number exponent bit mask */
#define F64_EXP_MASK U64(0x7FF00000, 0x00000000)


static Py_ssize_t _ControlJump[_Slash + 1] = {
        6, 6, 6, 6, 6, 6, 6, 6, 2, 2, 2, 6, 2, 2, 6, 6, // 0-15
        6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, // 16-31
        0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 32-47
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 48-63
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 64-79
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2           // 80-92
};


/*==============================================================================
 * Buffer
 *============================================================================*/

static_assert((PYYJSON_ENCODE_DST_BUFFER_INIT_SIZE % 64) == 0, "(PYYJSON_ENCODE_DST_BUFFER_INIT_SIZE % 64) == 0");

/*==============================================================================
 * Utils
 *============================================================================*/

force_inline Py_ssize_t get_indent_char_count(Py_ssize_t cur_nested_depth, Py_ssize_t indent_level) {
    return indent_level ? (indent_level * cur_nested_depth + 1) : 0;
}

/*==============================================================================
 * Python Utils
 *============================================================================*/

#define PYBYTES_START_OFFSET (offsetof(PyBytesObject, ob_sval))

extern PyObject *JSONEncodeError;

force_inline void *get_unicode_data(PyObject *unicode) {
    if (((PyASCIIObject *)unicode)->state.ascii) {
        return (void *)(((PyASCIIObject *)unicode) + 1);
    }
    return (void *)(((PyCompactUnicodeObject *)unicode) + 1);
}

force_inline bool pylong_is_unsigned(PyObject *obj) {
#if PY_MINOR_VERSION >= 12
    return !(bool)(((PyLongObject *)obj)->long_value.lv_tag & 2);
#else
    return ((PyVarObject *)obj)->ob_size > 0;
#endif
}

force_inline bool pylong_is_zero(PyObject *obj) {
#if PY_MINOR_VERSION >= 12
    return (bool)(((PyLongObject *)obj)->long_value.lv_tag & 1);
#else
    return ((PyVarObject *)obj)->ob_size == 0;
#endif
}

// PyErr may occur.
force_inline bool pylong_value_unsigned(PyObject *obj, u64 *value) {
#if PY_MINOR_VERSION >= 12
    if (likely(((PyLongObject *)obj)->long_value.lv_tag < (2 << _PyLong_NON_SIZE_BITS))) {
        *value = (u64) * ((PyLongObject *)obj)->long_value.ob_digit;
        return true;
    }
#endif
    unsigned long long v = PyLong_AsUnsignedLongLong(obj);
    if (unlikely(v == (unsigned long long)-1 && PyErr_Occurred())) {
        return false;
    }
    *value = (u64)v;
    static_assert(sizeof(unsigned long long) <= sizeof(u64), "sizeof(unsigned long long) <= sizeof(u64)");
    return true;
}

force_inline i64 pylong_value_signed(PyObject *obj, i64 *value) {
#if PY_MINOR_VERSION >= 12
    if (likely(((PyLongObject *)obj)->long_value.lv_tag < (2 << _PyLong_NON_SIZE_BITS))) {
        i64 sign = 1 - (i64)((((PyLongObject *)obj)->long_value.lv_tag & 3));
        *value = sign * (i64) * ((PyLongObject *)obj)->long_value.ob_digit;
        return true;
    }
#endif
    long long v = PyLong_AsLongLong(obj);
    if (unlikely(v == -1 && PyErr_Occurred())) {
        return false;
    }
    *value = (i64)v;
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
 * Writer
 *============================================================================*/

/* These codes are modified from yyjson. */


/** Digit table from 00 to 99. */
extern pyyjson_align(8) const char DIGIT_TABLE[200];

/** Normalized significant 128 bits of pow10, no rounded up (size: 10.4KB).
    This lookup table is used by both the double number reader and writer.
    (generate with misc/make_tables.c) */
extern const u64 pow10_sig_table[];

force_inline void byte_copy_2(void *dst, const void *src) {
    memcpy(dst, src, 2);
}

force_inline void byte_copy_4(void *dst, const void *src) {
    memcpy(dst, src, 4);
}

force_inline void byte_copy_8(void *dst, const void *src) {
    memcpy(dst, src, 8);
}

/*==============================================================================
 * Number Utils
 * These functions are used to detect and convert NaN and Inf numbers.
 *============================================================================*/

/** Convert raw binary to double. */
force_inline f64 f64_from_raw(u64 u) {
    /* use memcpy to avoid violating the strict aliasing rule */
    f64 f;
    memcpy(&f, &u, 8);
    return f;
}

// /**
//  Get the cached pow10 value from pow10_sig_table.
//  @param exp10 The exponent of pow(10, e). This value must in range
//               POW10_SIG_TABLE_MIN_EXP to POW10_SIG_TABLE_MAX_EXP.
//  @param hi    The highest 64 bits of pow(10, e).
//  @param lo    The lower 64 bits after `hi`.
//  */
// force_inline void pow10_table_get_sig(i32 exp10, u64 *hi, u64 *lo) {
//     i32 idx = exp10 - (POW10_SIG_TABLE_MIN_EXP);
//     *hi = pow10_sig_table[idx * 2];
//     *lo = pow10_sig_table[idx * 2 + 1];
// }

/*==============================================================================
 * Integer Writer
 *
 * The maximum value of uint32_t is 4294967295 (10 digits),
 * these digits are named as 'aabbccddee' here.
 *
 * Although most compilers may convert the "division by constant value" into
 * "multiply and shift", manual conversion can still help some compilers
 * generate fewer and better instructions.
 *
 * Reference:
 * Division by Invariant Integers using Multiplication, 1994.
 * https://gmplib.org/~tege/divcnst-pldi94.pdf
 * Improved division by invariant integers, 2011.
 * https://gmplib.org/~tege/division-paper.pdf
 *============================================================================*/

force_inline u8 *write_u32_len_8(u32 val, u8 *buf) {
    u32 aa, bb, cc, dd, aabb, ccdd;             /* 8 digits: aabbccdd */
    aabb = (u32)(((u64)val * 109951163) >> 40); /* (val / 10000) */
    ccdd = val - aabb * 10000;                  /* (val % 10000) */
    aa = (aabb * 5243) >> 19;                   /* (aabb / 100) */
    cc = (ccdd * 5243) >> 19;                   /* (ccdd / 100) */
    bb = aabb - aa * 100;                       /* (aabb % 100) */
    dd = ccdd - cc * 100;                       /* (ccdd % 100) */
    byte_copy_2(buf + 0, DIGIT_TABLE + aa * 2);
    byte_copy_2(buf + 2, DIGIT_TABLE + bb * 2);
    byte_copy_2(buf + 4, DIGIT_TABLE + cc * 2);
    byte_copy_2(buf + 6, DIGIT_TABLE + dd * 2);
    return buf + 8;
}

force_inline u8 *write_u32_len_4(u32 val, u8 *buf) {
    u32 aa, bb;              /* 4 digits: aabb */
    aa = (val * 5243) >> 19; /* (val / 100) */
    bb = val - aa * 100;     /* (val % 100) */
    byte_copy_2(buf + 0, DIGIT_TABLE + aa * 2);
    byte_copy_2(buf + 2, DIGIT_TABLE + bb * 2);
    return buf + 4;
}

force_inline u8 *write_u32_len_1_8(u32 val, u8 *buf) {
    u32 aa, bb, cc, dd, aabb, bbcc, ccdd, lz;

    if (val < 100) {   /* 1-2 digits: aa */
        lz = val < 10; /* leading zero: 0 or 1 */
        byte_copy_2(buf + 0, DIGIT_TABLE + val * 2 + lz);
        buf -= lz;
        return buf + 2;

    } else if (val < 10000) {    /* 3-4 digits: aabb */
        aa = (val * 5243) >> 19; /* (val / 100) */
        bb = val - aa * 100;     /* (val % 100) */
        lz = aa < 10;            /* leading zero: 0 or 1 */
        byte_copy_2(buf + 0, DIGIT_TABLE + aa * 2 + lz);
        buf -= lz;
        byte_copy_2(buf + 2, DIGIT_TABLE + bb * 2);
        return buf + 4;

    } else if (val < 1000000) {                /* 5-6 digits: aabbcc */
        aa = (u32)(((u64)val * 429497) >> 32); /* (val / 10000) */
        bbcc = val - aa * 10000;               /* (val % 10000) */
        bb = (bbcc * 5243) >> 19;              /* (bbcc / 100) */
        cc = bbcc - bb * 100;                  /* (bbcc % 100) */
        lz = aa < 10;                          /* leading zero: 0 or 1 */
        byte_copy_2(buf + 0, DIGIT_TABLE + aa * 2 + lz);
        buf -= lz;
        byte_copy_2(buf + 2, DIGIT_TABLE + bb * 2);
        byte_copy_2(buf + 4, DIGIT_TABLE + cc * 2);
        return buf + 6;

    } else {                                        /* 7-8 digits: aabbccdd */
        aabb = (u32)(((u64)val * 109951163) >> 40); /* (val / 10000) */
        ccdd = val - aabb * 10000;                  /* (val % 10000) */
        aa = (aabb * 5243) >> 19;                   /* (aabb / 100) */
        cc = (ccdd * 5243) >> 19;                   /* (ccdd / 100) */
        bb = aabb - aa * 100;                       /* (aabb % 100) */
        dd = ccdd - cc * 100;                       /* (ccdd % 100) */
        lz = aa < 10;                               /* leading zero: 0 or 1 */
        byte_copy_2(buf + 0, DIGIT_TABLE + aa * 2 + lz);
        buf -= lz;
        byte_copy_2(buf + 2, DIGIT_TABLE + bb * 2);
        byte_copy_2(buf + 4, DIGIT_TABLE + cc * 2);
        byte_copy_2(buf + 6, DIGIT_TABLE + dd * 2);
        return buf + 8;
    }
}

force_inline u8 *write_u64_len_5_8(u32 val, u8 *buf) {
    u32 aa, bb, cc, dd, aabb, bbcc, ccdd, lz;

    if (val < 1000000) {                       /* 5-6 digits: aabbcc */
        aa = (u32)(((u64)val * 429497) >> 32); /* (val / 10000) */
        bbcc = val - aa * 10000;               /* (val % 10000) */
        bb = (bbcc * 5243) >> 19;              /* (bbcc / 100) */
        cc = bbcc - bb * 100;                  /* (bbcc % 100) */
        lz = aa < 10;                          /* leading zero: 0 or 1 */
        byte_copy_2(buf + 0, DIGIT_TABLE + aa * 2 + lz);
        buf -= lz;
        byte_copy_2(buf + 2, DIGIT_TABLE + bb * 2);
        byte_copy_2(buf + 4, DIGIT_TABLE + cc * 2);
        return buf + 6;

    } else {                                        /* 7-8 digits: aabbccdd */
        aabb = (u32)(((u64)val * 109951163) >> 40); /* (val / 10000) */
        ccdd = val - aabb * 10000;                  /* (val % 10000) */
        aa = (aabb * 5243) >> 19;                   /* (aabb / 100) */
        cc = (ccdd * 5243) >> 19;                   /* (ccdd / 100) */
        bb = aabb - aa * 100;                       /* (aabb % 100) */
        dd = ccdd - cc * 100;                       /* (ccdd % 100) */
        lz = aa < 10;                               /* leading zero: 0 or 1 */
        byte_copy_2(buf + 0, DIGIT_TABLE + aa * 2 + lz);
        buf -= lz;
        byte_copy_2(buf + 2, DIGIT_TABLE + bb * 2);
        byte_copy_2(buf + 4, DIGIT_TABLE + cc * 2);
        byte_copy_2(buf + 6, DIGIT_TABLE + dd * 2);
        return buf + 8;
    }
}

force_inline u8 *write_u64(u64 val, u8 *buf) {
    u64 tmp, hgh;
    u32 mid, low;

    if (val < 100000000) { /* 1-8 digits */
        buf = write_u32_len_1_8((u32)val, buf);
        return buf;

    } else if (val < (u64)100000000 * 100000000) { /* 9-16 digits */
        hgh = val / 100000000;                     /* (val / 100000000) */
        low = (u32)(val - hgh * 100000000);        /* (val % 100000000) */
        buf = write_u32_len_1_8((u32)hgh, buf);
        buf = write_u32_len_8(low, buf);
        return buf;

    } else {                                /* 17-20 digits */
        tmp = val / 100000000;              /* (val / 100000000) */
        low = (u32)(val - tmp * 100000000); /* (val % 100000000) */
        hgh = (u32)(tmp / 10000);           /* (tmp / 10000) */
        mid = (u32)(tmp - hgh * 10000);     /* (tmp % 10000) */
        buf = write_u64_len_5_8((u32)hgh, buf);
        buf = write_u32_len_4(mid, buf);
        buf = write_u32_len_8(low, buf);
        return buf;
    }
}

#endif // PYYJSON_ENCODE_SHARED_H
