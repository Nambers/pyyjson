#ifndef PYYJSON_DECODE_H
#define PYYJSON_DECODE_H
#include "pyyjson.h"
#include "simd/memcmp.h"
#include "simd/simd_impl.h"
#include "xxhash.h"

typedef struct DecodeObjStackInfo {
    PyObject **cur_write_result_addr;
    PyObject **result_stack;
    PyObject **result_stack_end;
} DecodeObjStackInfo;

typedef struct DecodeCtnWithSize DecodeCtnWithSize;

typedef struct DecodeCtnStackInfo {
    DecodeCtnWithSize *ctn;
    DecodeCtnWithSize *ctn_start;
    DecodeCtnWithSize *ctn_end;
} DecodeCtnStackInfo;

typedef union {
    struct {
        u32 escape_val;
        u32 escape_size;
    };

    u64 union_value;
} EscapeInfo;

#define _DECODE_UNICODE_ERR PYYJSON_CAST(u32, -1)

#define DECODE_LOOPSTATE_CONTINUE 0
#define DECODE_LOOPSTATE_END 1
#define DECODE_LOOPSTATE_ESCAPE 2
#define DECODE_LOOPSTATE_INVALID 3

extern PyObject *JSONDecodeError;

typedef enum ReadStrScanFlag {
    StrContinue,
    StrInvalid,
    StrEnd,
} ReadStrScanFlag;

typedef struct ReadStrState {
    ReadStrScanFlag scan_flag;
    int max_char_type;
    bool need_copy;
    bool dont_check_max_char;
    bool state_dirty;
} ReadStrState;

typedef struct SpecialCharReadResult {
    u32 value;
    ReadStrScanFlag flag;
} SpecialCharReadResult;

/*==============================================================================
 * Integer Constants
 *============================================================================*/

// /* Used to write u64 literal for C89 which doesn't support "ULL" suffix. */
// #undef U64
// #define U64(hi, lo) ((((u64)hi##UL) << 32U) + lo##UL)

/* U64 constant values */
#undef U64_MAX
#define U64_MAX U64(0xFFFFFFFF, 0xFFFFFFFF)
#undef I64_MAX
#define I64_MAX U64(0x7FFFFFFF, 0xFFFFFFFF)
#undef USIZE_MAX
#define USIZE_MAX ((usize)(~(usize)0))

/* Maximum number of digits for reading u32/u64/usize safety (not overflow). */
#undef U32_SAFE_DIG
#define U32_SAFE_DIG 9 /* u32 max is 4294967295, 10 digits */
#undef U64_SAFE_DIG
#define U64_SAFE_DIG 19 /* u64 max is 18446744073709551615, 20 digits */
#undef USIZE_SAFE_DIG
#define USIZE_SAFE_DIG (sizeof(usize) == 8 ? U64_SAFE_DIG : U32_SAFE_DIG)


/*==============================================================================
 * IEEE-754 Double Number Constants
 *============================================================================*/

/* Inf raw value (positive) */
#define F64_RAW_INF U64(0x7FF00000, 0x00000000)

/* NaN raw value (quiet NaN, no payload, no sign) */
#if defined(__hppa__) || (defined(__mips__) && !defined(__mips_nan2008))
#    define F64_RAW_NAN U64(0x7FF7FFFF, 0xFFFFFFFF)
#else
#    define F64_RAW_NAN U64(0x7FF80000, 0x00000000)
#endif

/* double number bits */
#define F64_BITS 64

/* double number exponent part bits */
#define F64_EXP_BITS 11

/* double number significand part bits */
#define F64_SIG_BITS 52

/* double number significand part bits (with 1 hidden bit) */
#define F64_SIG_FULL_BITS 53

/* double number significand bit mask */
#define F64_SIG_MASK U64(0x000FFFFF, 0xFFFFFFFF)

/* double number exponent bit mask */
#define F64_EXP_MASK U64(0x7FF00000, 0x00000000)

/* double number exponent bias */
#define F64_EXP_BIAS 1023

/* double number significant digits count in decimal */
#define F64_DEC_DIG 17

/* max significant digits count in decimal when reading double number */
#define F64_MAX_DEC_DIG 768

/* maximum decimal power of double number (1.7976931348623157e308) */
#define F64_MAX_DEC_EXP 308

/* minimum decimal power of double number (4.9406564584124654e-324) */
#define F64_MIN_DEC_EXP (-324)

/* maximum binary power of double number */
#define F64_MAX_BIN_EXP 1024

/* minimum binary power of double number */
#define F64_MIN_BIN_EXP (-1021)

/*==============================================================================
 * Hex Character Reader
 * This function is used by JSON reader to read escaped characters.
 *============================================================================*/


/**
 Scans an escaped character sequence as a UTF-16 code unit (branchless).
 e.g. "\\u005C" should pass "005C" as `cur`.
 
 This requires the string has 4-byte zero padding.
 */
// force_inline bool read_8_to_hex_u16(const u8 *cur, u16 *val);

force_inline bool byte_match_2(const void *buf, const void *pat) {
    u16 u1, u2;
    memcpy(&u1, buf, 2);
    memcpy(&u2, pat, 2);
    return u1 == u2;
}

force_inline bool byte_match_4(const void *buf, const void *pat) {
    u32 u1, u2;
    memcpy(&u1, buf, 4);
    memcpy(&u2, pat, 4);
    return u1 == u2;
}

force_inline void byte_move_2(void *dst, const void *src) {
    u16 tmp;
    memcpy(&tmp, src, 2);
    memcpy(dst, &tmp, 2);
}

force_inline void byte_move_4(void *dst, const void *src) {
    u32 tmp;
    memcpy(&tmp, src, 4);
    memcpy(dst, &tmp, 4);
}

force_inline void byte_move_8(void *dst, const void *src) {
    u64 tmp;
    memcpy(&tmp, src, 8);
    memcpy(dst, &tmp, 8);
}

// force_inline void byte_move_16(void *dst, const void *src) {
// char *pdst = (char *)dst;
// const char *psrc = (const char *)src;
// u64 tmp1, tmp2;
// memcpy(&tmp1, psrc, 8);
// memcpy(&tmp2, psrc + 8, 8);
// memcpy(pdst, &tmp1, 8);
// memcpy(pdst + 8, &tmp2, 8);
// }

force_inline u32 byte_load_4(const void *src) {
    u32 u;
    memcpy(&u, src, 4);
    return u;
}

/*==============================================================================
 * JSON Reader Utils
 * These functions are used by JSON reader to read literals and comments.
 *============================================================================*/

force_inline bool pyyjson_decode_nan(DecodeObjStackInfo *restrict decode_obj_stack_info, bool is_signed);


/*==============================================================================
 * Power10 Lookup Table
 * These data are used by the floating-point number reader and writer.
 *============================================================================*/

/** Normalized significant 128 bits of pow10, no rounded up (size: 10.4KB).
    This lookup table is used by both the double number reader and writer.
    (generate with misc/make_tables.c) */
extern const u64 pow10_sig_table[];

/**
 Convert normalized u64 (highest bit is 1) to f64.
 
 Some compiler (such as Microsoft Visual C++ 6.0) do not support converting
 number from u64 to f64. This function will first convert u64 to i64 and then
 to f64, with `to nearest` rounding mode.
 */
force_inline f64 normalized_u64_to_f64(u64 val) {
#if PYYJSON_U64_TO_F64_NO_IMPL
    i64 sig = (i64)((val >> 1) | (val & 1));
    return ((f64)sig) * (f64)2.0;
#else
    return (f64)val;
#endif
}

/*==============================================================================
 * Read state utilities
 *============================================================================*/

force_inline void update_max_char_type(ReadStrState *read_state, int max_char_type) {
    assert(read_state->max_char_type < max_char_type);
    read_state->max_char_type = max_char_type;
    read_state->state_dirty = true;
}

force_inline void init_read_state(ReadStrState *state) {
    // all initialized as 0 or false
    memset(state, 0, sizeof(ReadStrState));
}

/*==============================================================================
 * xxhash and key cache utilities
 *============================================================================*/
#if PY_MINOR_VERSION >= 13
// these are hidden in Python 3.13
#    if PY_MINOR_VERSION == 13
PyAPI_FUNC(Py_hash_t) _Py_HashBytes(const void *, Py_ssize_t);
#    endif // PY_MINOR_VERSION == 13
PyAPI_FUNC(int) _PyDict_SetItem_KnownHash_LockHeld(PyObject *mp, PyObject *key, PyObject *item, Py_hash_t hash);
#    define _PyDict_SetItem_KnownHash _PyDict_SetItem_KnownHash_LockHeld
#endif // PY_MINOR_VERSION >= 13

#define REHASHER(_x) (((size_t)(_x)) % (PYYJSON_KEY_CACHE_SIZE))
typedef XXH64_hash_t pyyjson_hash_t;
extern pyyjson_cache_type AssociativeKeyCache[PYYJSON_KEY_CACHE_SIZE];

force_inline void add_key_cache(pyyjson_hash_t hash, PyObject *obj) {
    assert(PyUnicode_GET_LENGTH(obj) * PyUnicode_KIND(obj) <= 64);
    size_t index = REHASHER(hash);
    // PYYJSON_TRACE_HASH(index);
    pyyjson_cache_type old = AssociativeKeyCache[index];
    if (old) {
        // PYYJSON_TRACE_HASH_CONFLICT(hash);
        Py_DECREF(old);
    }
    Py_INCREF(obj);
    AssociativeKeyCache[index] = obj;
}

force_inline PyObject *get_key_cache(const u8 *unicode_str, pyyjson_hash_t hash, size_t real_len, int kind, bool ascii) {
    assert(real_len <= 64);
    pyyjson_cache_type cache = AssociativeKeyCache[REHASHER(hash)];
    if (!cache) return NULL;
    PyASCIIObject *cache_ascii = PYYJSON_CAST(PyASCIIObject *, cache);
    Py_ssize_t cache_length = cache_ascii->length;
    Py_ssize_t cache_kind = cache_ascii->state.kind;
    bool cache_is_ascii = cache_ascii->state.ascii;
    Py_ssize_t cache_offset = cache_is_ascii ? sizeof(PyASCIIObject) : sizeof(PyCompactUnicodeObject);
    if (likely(kind == cache_kind && ascii == cache_is_ascii && ((real_len == cache_length * cache_kind)) && (pyyjson_memcmp_neq_le64(PYYJSON_CAST(u8 *, unicode_str), PYYJSON_CAST(u8 *, cache) + cache_offset, real_len) == 0))) {
        // PYYJSON_TRACE_CACHE_HIT();
        return cache;
    }
    return NULL;
}

force_inline void make_hash(PyASCIIObject *ascii, const void *unicode_str, size_t real_len) {
#if PY_MINOR_VERSION >= 14
    ascii->hash = Py_HashBuffer(unicode_str, real_len);
#else
    ascii->hash = _Py_HashBytes(unicode_str, real_len);
#endif
}

#endif // PYYJSON_DECODE_H
