#include "decode.h"


/**
 Microsoft Visual C++ 6.0 doesn't support converting number from u64 to f64:
 error C2520: conversion from unsigned __int64 to double not implemented.
 */
#ifndef PYYJSON_U64_TO_F64_NO_IMPL
#   if (0 < PYYJSON_MSC_VER) && (PYYJSON_MSC_VER <= 1200)
#       define PYYJSON_U64_TO_F64_NO_IMPL 1
#   else
#       define PYYJSON_U64_TO_F64_NO_IMPL 0
#   endif
#endif

/* int128 type */
#if defined(__SIZEOF_INT128__) && (__SIZEOF_INT128__ == 16) && \
    (defined(__GNUC__) || defined(__clang__) || defined(__INTEL_COMPILER))
#    define PYYJSON_HAS_INT128 1
/** 128-bit integer, used by floating-point number reader and writer. */
__extension__ typedef __int128          i128;
__extension__ typedef unsigned __int128 u128;
#else
#    define PYYJSON_HAS_INT128 0
#endif

/*
 Correct rounding in double number computations.
 
 On the x86 architecture, some compilers may use x87 FPU instructions for
 floating-point arithmetic. The x87 FPU loads all floating point number as
 80-bit double-extended precision internally, then rounds the result to original
 precision, which may produce inaccurate results. For a more detailed
 explanation, see the paper: https://arxiv.org/abs/cs/0701192
 
 Here are some examples of double precision calculation error:
 
     2877.0 / 1e6   == 0.002877,  but x87 returns 0.0028770000000000002
     43683.0 * 1e21 == 4.3683e25, but x87 returns 4.3683000000000004e25
 
 Here are some examples of compiler flags to generate x87 instructions on x86:
 
     clang -m32 -mno-sse
     gcc/icc -m32 -mfpmath=387
     msvc /arch:SSE or /arch:IA32
 
 If we are sure that there's no similar error described above, we can define the
 PYYJSON_DOUBLE_MATH_CORRECT as 1 to enable the fast path calculation. This is
 not an accurate detection, it's just try to avoid the error at compile-time.
 An accurate detection can be done at run-time:
 
     bool is_double_math_correct(void) {
         volatile double r = 43683.0;
         r *= 1e21;
         return r == 4.3683e25;
     }
 
 See also: utils.h in https://github.com/google/double-conversion/
 */
#if !defined(FLT_EVAL_METHOD) && defined(__FLT_EVAL_METHOD__)
#    define FLT_EVAL_METHOD __FLT_EVAL_METHOD__
#endif

#if defined(FLT_EVAL_METHOD) && FLT_EVAL_METHOD != 0 && FLT_EVAL_METHOD != 1
#    define PYYJSON_DOUBLE_MATH_CORRECT 0
#elif defined(i386) || defined(__i386) || defined(__i386__) || \
    defined(_X86_) || defined(__X86__) || defined(_M_IX86) || \
    defined(__I86__) || defined(__IA32__) || defined(__THW_INTEL)
#   if (defined(_MSC_VER) && defined(_M_IX86_FP) && _M_IX86_FP == 2) || \
        (defined(__SSE2_MATH__) && __SSE2_MATH__)
#       define PYYJSON_DOUBLE_MATH_CORRECT 1
#   else
#       define PYYJSON_DOUBLE_MATH_CORRECT 0
#   endif
#elif defined(__mc68000__) || defined(__pnacl__) || defined(__native_client__)
#   define PYYJSON_DOUBLE_MATH_CORRECT 0
#else
#   define PYYJSON_DOUBLE_MATH_CORRECT 1
#endif

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

force_inline bool pyyjson_decode_double(DecodeObjStackInfo* restrict decode_obj_stack_info, double val);

force_inline bool pyyjson_decode_longlong(DecodeObjStackInfo* restrict decode_obj_stack_info, i64 val);

/*==============================================================================
 * 128-bit Integer Utils
 * These functions are used by the floating-point number reader and writer.
 *============================================================================*/

/** Multiplies two 64-bit unsigned integers (a * b),
    returns the 128-bit result as 'hi' and 'lo'. */
force_inline void u128_mul(u64 a, u64 b, u64 *hi, u64 *lo) {
#if PYYJSON_HAS_INT128
    u128 m = (u128)a * b;
    *hi = (u64)(m >> 64);
    *lo = (u64)(m);
#elif MSC_HAS_UMUL128
    *lo = _umul128(a, b, hi);
#else
    u32 a0 = (u32)(a), a1 = (u32)(a >> 32);
    u32 b0 = (u32)(b), b1 = (u32)(b >> 32);
    u64 p00 = (u64)a0 * b0, p01 = (u64)a0 * b1;
    u64 p10 = (u64)a1 * b0, p11 = (u64)a1 * b1;
    u64 m0 = p01 + (p00 >> 32);
    u32 m00 = (u32)(m0), m01 = (u32)(m0 >> 32);
    u64 m1 = p10 + m00;
    u32 m10 = (u32)(m1), m11 = (u32)(m1 >> 32);
    *hi = p11 + m01 + m11;
    *lo = ((u64)m10 << 32) | (u32)p00;
#endif
}

/** Multiplies two 64-bit unsigned integers and add a value (a * b + c),
    returns the 128-bit result as 'hi' and 'lo'. */
force_inline void u128_mul_add(u64 a, u64 b, u64 c, u64 *hi, u64 *lo) {
#if PYYJSON_HAS_INT128
    u128 m = (u128)a * b + c;
    *hi = (u64)(m >> 64);
    *lo = (u64)(m);
#else
    u64 h, l, t;
    u128_mul(a, b, &h, &l);
    t = l + c;
    h += (u64)(((t < l) | (t < c)));
    *hi = h;
    *lo = t;
#endif
}


#if PYYJSON_HAS_IEEE_754

/*==============================================================================
 * BigInt For Floating Point Number Reader
 *
 * The bigint algorithm is used by floating-point number reader to get correctly
 * rounded result for numbers with lots of digits. This part of code is rarely
 * used for common numbers.
 *============================================================================*/

/** Maximum exponent of exact pow10 */
#define U64_POW10_MAX_EXP 19

/** Table: [ 10^0, ..., 10^19 ] (generate with misc/make_tables.c) */
static const u64 u64_pow10_table[U64_POW10_MAX_EXP + 1] = {
    U64(0x00000000, 0x00000001), U64(0x00000000, 0x0000000A),
    U64(0x00000000, 0x00000064), U64(0x00000000, 0x000003E8),
    U64(0x00000000, 0x00002710), U64(0x00000000, 0x000186A0),
    U64(0x00000000, 0x000F4240), U64(0x00000000, 0x00989680),
    U64(0x00000000, 0x05F5E100), U64(0x00000000, 0x3B9ACA00),
    U64(0x00000002, 0x540BE400), U64(0x00000017, 0x4876E800),
    U64(0x000000E8, 0xD4A51000), U64(0x00000918, 0x4E72A000),
    U64(0x00005AF3, 0x107A4000), U64(0x00038D7E, 0xA4C68000),
    U64(0x002386F2, 0x6FC10000), U64(0x01634578, 0x5D8A0000),
    U64(0x0DE0B6B3, 0xA7640000), U64(0x8AC72304, 0x89E80000)
};

/** Maximum numbers of chunks used by a bigint (58 is enough here). */
#define BIGINT_MAX_CHUNKS 64

/** Unsigned arbitrarily large integer */
typedef struct bigint {
    u32 used; /* used chunks count, should not be 0 */
    u64 bits[BIGINT_MAX_CHUNKS]; /* chunks */
} bigint;

/**
 Evaluate 'big += val'.
 @param big A big number (can be 0).
 @param val An unsigned integer (can be 0).
 */
force_inline void bigint_add_u64(bigint *big, u64 val) {
    u32 idx, max;
    u64 num = big->bits[0];
    u64 add = num + val;
    big->bits[0] = add;
    if (likely((add >= num) || (add >= val))) return;
    for ((void)(idx = 1), max = big->used; idx < max; idx++) {
        if (likely(big->bits[idx] != U64_MAX)) {
            big->bits[idx] += 1;
            return;
        }
        big->bits[idx] = 0;
    }
    big->bits[big->used++] = 1;
}

/**
 Evaluate 'big *= val'.
 @param big A big number (can be 0).
 @param val An unsigned integer (cannot be 0).
 */
force_inline void bigint_mul_u64(bigint *big, u64 val) {
    u32 idx = 0, max = big->used;
    u64 hi, lo, carry = 0;
    for (; idx < max; idx++) {
        if (big->bits[idx]) break;
    }
    for (; idx < max; idx++) {
        u128_mul_add(big->bits[idx], val, carry, &hi, &lo);
        big->bits[idx] = lo;
        carry = hi;
    }
    if (carry) big->bits[big->used++] = carry;
}

/**
 Evaluate 'big *= 2^exp'.
 @param big A big number (can be 0).
 @param exp An exponent integer (can be 0).
 */
force_inline void bigint_mul_pow2(bigint *big, u32 exp) {
    u32 shft = exp % 64;
    u32 move = exp / 64;
    u32 idx = big->used;
    if (unlikely(shft == 0)) {
        for (; idx > 0; idx--) {
            big->bits[idx + move - 1] = big->bits[idx - 1];
        }
        big->used += move;
        while (move) big->bits[--move] = 0;
    } else {
        big->bits[idx] = 0;
        for (; idx > 0; idx--) {
            u64 num = big->bits[idx] << shft;
            num |= big->bits[idx - 1] >> (64 - shft);
            big->bits[idx + move] = num;
        }
        big->bits[move] = big->bits[0] << shft;
        big->used += move + (big->bits[big->used + move] > 0);
        while (move) big->bits[--move] = 0;
    }
}

/**
 Evaluate 'big *= 10^exp'.
 @param big A big number (can be 0).
 @param exp An exponent integer (cannot be 0).
 */
force_inline void bigint_mul_pow10(bigint *big, i32 exp) {
    for (; exp >= U64_POW10_MAX_EXP; exp -= U64_POW10_MAX_EXP) {
        bigint_mul_u64(big, u64_pow10_table[U64_POW10_MAX_EXP]);
    }
    if (exp) {
        bigint_mul_u64(big, u64_pow10_table[exp]);
    }
}

/**
 Compare two bigint.
 @return -1 if 'a < b', +1 if 'a > b', 0 if 'a == b'.
 */
force_inline i32 bigint_cmp(bigint *a, bigint *b) {
    u32 idx = a->used;
    if (a->used < b->used) return -1;
    if (a->used > b->used) return +1;
    while (idx-- > 0) {
        u64 av = a->bits[idx];
        u64 bv = b->bits[idx];
        if (av < bv) return -1;
        if (av > bv) return +1;
    }
    return 0;
}

/**
 Evaluate 'big = val'.
 @param big A big number (can be 0).
 @param val An unsigned integer (can be 0).
 */
force_inline void bigint_set_u64(bigint *big, u64 val) {
    big->used = 1;
    big->bits[0] = val;
}

/** Set a bigint with floating point number string. */
static force_noinline void bigint_set_buf(bigint *big, u64 sig, i32 *exp,
                                    u8 *sig_cut, u8 *sig_end, u8 *dot_pos) {
    
    if (unlikely(!sig_cut)) {
        /* no digit cut, set significant part only */
        bigint_set_u64(big, sig);
        return;
        
    } else {
        /* some digits were cut, read them from 'sig_cut' to 'sig_end' */
        u8 *hdr = sig_cut;
        u8 *cur = hdr;
        u32 len = 0;
        u64 val = 0;
        bool dig_big_cut = false;
        bool has_dot = (hdr < dot_pos) & (dot_pos < sig_end);
        u32 dig_len_total = U64_SAFE_DIG + (u32)(sig_end - hdr) - has_dot;
        
        sig -= (*sig_cut >= '5'); /* sig was rounded before */
        if (dig_len_total > F64_MAX_DEC_DIG) {
            dig_big_cut = true;
            sig_end -= dig_len_total - (F64_MAX_DEC_DIG + 1);
            sig_end -= (dot_pos + 1 == sig_end);
            dig_len_total = (F64_MAX_DEC_DIG + 1);
        }
        *exp -= (i32)dig_len_total - U64_SAFE_DIG;
        
        big->used = 1;
        big->bits[0] = sig;
        while (cur < sig_end) {
            if (likely(cur != dot_pos)) {
                val = val * 10 + (u8)(*cur++ - '0');
                len++;
                if (unlikely(cur == sig_end && dig_big_cut)) {
                    /* The last digit must be non-zero,    */
                    /* set it to '1' for correct rounding. */
                    val = val - (val % 10) + 1;
                }
                if (len == U64_SAFE_DIG || cur == sig_end) {
                    bigint_mul_pow10(big, (i32)len);
                    bigint_add_u64(big, val);
                    val = 0;
                    len = 0;
                }
            } else {
                cur++;
            }
        }
    }
}



/*==============================================================================
 * Diy Floating Point
 *============================================================================*/

/** "Do It Yourself Floating Point" struct. */
typedef struct diy_fp {
    u64 sig; /* significand */
    i32 exp; /* exponent, base 2 */
    i32 pad; /* padding, useless */
} diy_fp;

/** Get cached rounded diy_fp with pow(10, e) The input value must in range
    [POW10_SIG_TABLE_MIN_EXP, POW10_SIG_TABLE_MAX_EXP]. */
force_inline diy_fp diy_fp_get_cached_pow10(i32 exp10) {
    diy_fp fp;
    u64 sig_ext;
    pow10_table_get_sig(exp10, &fp.sig, &sig_ext);
    pow10_table_get_exp(exp10, &fp.exp);
    fp.sig += (sig_ext >> 63);
    return fp;
}

/** Returns fp * fp2. */
force_inline diy_fp diy_fp_mul(diy_fp fp, diy_fp fp2) {
    u64 hi, lo;
    u128_mul(fp.sig, fp2.sig, &hi, &lo);
    fp.sig = hi + (lo >> 63);
    fp.exp += fp2.exp + 64;
    return fp;
}

/** Convert diy_fp to IEEE-754 raw value. */
force_inline u64 diy_fp_to_ieee_raw(diy_fp fp) {
    u64 sig = fp.sig;
    i32 exp = fp.exp;
    u32 lz_bits;
    if (unlikely(fp.sig == 0)) return 0;
    
    lz_bits = u64_lz_bits(sig);
    sig <<= lz_bits;
    sig >>= F64_BITS - F64_SIG_FULL_BITS;
    exp -= (i32)lz_bits;
    exp += F64_BITS - F64_SIG_FULL_BITS;
    exp += F64_SIG_BITS;
    
    if (unlikely(exp >= F64_MAX_BIN_EXP)) {
        /* overflow */
        return F64_RAW_INF;
    } else if (likely(exp >= F64_MIN_BIN_EXP - 1)) {
        /* normal */
        exp += F64_EXP_BIAS;
        return ((u64)exp << F64_SIG_BITS) | (sig & F64_SIG_MASK);
    } else if (likely(exp >= F64_MIN_BIN_EXP - F64_SIG_FULL_BITS)) {
        /* subnormal */
        return sig >> (F64_MIN_BIN_EXP - exp - 1);
    } else {
        /* underflow */
        return 0;
    }
}



/*==============================================================================
 * JSON Number Reader (IEEE-754)
 *============================================================================*/

/** Maximum exact pow10 exponent for double value. */
#define F64_POW10_EXP_MAX_EXACT 22

/** Cached pow10 table. */
static const f64 f64_pow10_table[] = {
    1e0, 1e1, 1e2, 1e3, 1e4, 1e5, 1e6, 1e7, 1e8, 1e9, 1e10, 1e11, 1e12,
    1e13, 1e14, 1e15, 1e16, 1e17, 1e18, 1e19, 1e20, 1e21, 1e22
};

/**
 Read a JSON number.
 
 1. This function assume that the floating-point number is in IEEE-754 format.
 2. This function support uint64/int64/double number. If an integer number
    cannot fit in uint64/int64, it will returns as a double number. If a double
    number is infinite, the return value is based on flag.
 3. This function (with inline attribute) may generate a lot of instructions.
 */
force_inline PyObject* read_number(u8 **ptr) {

#define return_err(_end, _msg)                                                  \
    do {                                                                        \
        PyErr_Format(JSONDecodeError, "%s, at position %zu", _msg, _end - hdr); \
        return NULL;                                                           \
    } while (0)

#define return_0() do { \
    *end = cur; \
    return PyLong_FromLongLong(0); \
} while (false)

#define return_i64(_v) do { \
    *end = cur; \
    return PyLong_FromLongLong((i64)(sign ? (u64)(~(_v) + 1) : (u64)(_v))); \
} while (false)

#define return_f64(_v) do { \
    *end = cur; \
    return PyFloat_FromDouble(sign ? -(f64)(_v) : (f64)(_v)); \
} while (false)

#define return_f64_bin(_v) do { \
    *end = cur; \
    u64 temp = ((u64)sign << 63) | (u64)(_v); \
    return PyFloat_FromDouble(*(double *)&temp); \
} while (false)

#define return_inf() do { \
    return_f64_bin(F64_RAW_INF); \
} while (false)

    u8 *sig_cut = NULL; /* significant part cutting position for long number */
    u8 *sig_end = NULL; /* significant part ending position */
    u8 *dot_pos = NULL; /* decimal point position */
    
    u64 sig = 0; /* significant part of the number */
    i32 exp = 0; /* exponent part of the number */
    
    bool exp_sign; /* temporary exponent sign from literal part */
    i64 exp_sig = 0; /* temporary exponent number from significant part */
    i64 exp_lit = 0; /* temporary exponent number from exponent literal part */
    u64 num; /* temporary number for reading */
    u8 *tmp; /* temporary cursor for reading */
    
    u8 *hdr = (u8 *)*ptr;
    u8 *cur = (u8 *)*ptr;
    u8 **end = (u8 **)ptr;
    bool sign;
    
    /* read number as raw string if has `YYJSON_READ_NUMBER_AS_RAW` flag */
    // if (has_read_flag(NUMBER_AS_RAW)) {
    //     return read_number_raw(ptr, pre, flg, val, msg);
    // }
    
    sign = (*hdr == '-');
    cur += sign;
    
    /* begin with a leading zero or non-digit */
    if (unlikely(!digi_is_nonzero(*cur))) { /* 0 or non-digit char */
        if (unlikely(*cur != '0')) { /* non-digit char */
            //if (has_read_flag(ALLOW_INF_AND_NAN)) {
            PyObject* number_obj = read_inf_or_nan(sign, &cur);
            if (likely(number_obj)) {
                *end = cur;
                return number_obj;
            }
            //}
            if(unlikely(!PyErr_Occurred())){
                return_err(cur, "no digit after minus sign");
            }
            return NULL;
        }
        /* begin with 0 */
        if (likely(!digi_is_digit_or_fp(*++cur))) return_0();
        if (likely(*cur == '.')) {
            dot_pos = cur++;
            if (unlikely(!digi_is_digit(*cur))) {
                return_err(cur, "no digit after decimal point");
            }
            while (unlikely(*cur == '0')) cur++;
            if (likely(digi_is_digit(*cur))) {
                /* first non-zero digit after decimal point */
                sig = (u64)(*cur - '0'); /* read first digit */
                cur--;
                goto digi_frac_1; /* continue read fraction part */
            }
        }
        if (unlikely(digi_is_digit(*cur))) {
            return_err(cur - 1, "number with leading zero is not allowed");
        }
        if (unlikely(digi_is_exp(*cur))) { /* 0 with any exponent is still 0 */
            cur += (usize)1 + digi_is_sign(cur[1]);
            if (unlikely(!digi_is_digit(*cur))) {
                return_err(cur, "no digit after exponent sign");
            }
            while (digi_is_digit(*++cur));
        }
        return_f64_bin(0);
    }
    
    /* begin with non-zero digit */
    sig = (u64)(*cur - '0');
    
    /*
     Read integral part, same as the following code.
     
         for (int i = 1; i <= 18; i++) {
            num = cur[i] - '0';
            if (num <= 9) sig = num + sig * 10;
            else goto digi_sepr_i;
         }
     */
#define expr_intg(i) \
    if (likely((num = (u64)(cur[i] - (u8)'0')) <= 9)) sig = num + sig * 10; \
    else { goto digi_sepr_##i; }
    REPEAT_INCR_IN_1_18(expr_intg)
#undef expr_intg
    
    
    cur += 19; /* skip continuous 19 digits */
    if (!digi_is_digit_or_fp(*cur)) {
        /* this number is an integer consisting of 19 digits */
        if (sign && (sig > ((u64)1 << 63))) { /* overflow */
            // if (has_read_flag(BIGNUM_AS_RAW)) return_raw();
            return_f64(normalized_u64_to_f64(sig));
        }
        return_i64(sig);
    }
    goto digi_intg_more; /* read more digits in integral part */
    
    
    /* process first non-digit character */
#define expr_sepr(i) \
    digi_sepr_##i: \
    if (likely(!digi_is_fp(cur[i]))) { cur += i; return_i64(sig); } \
    dot_pos = cur + i; \
    if (likely(cur[i] == '.')) goto digi_frac_##i; \
    cur += i; sig_end = cur; goto digi_exp_more;
    REPEAT_INCR_IN_1_18(expr_sepr)
#undef expr_sepr
    
    
    /* read fraction part */
#define expr_frac(i) \
    digi_frac_##i: \
    if (likely((num = (u64)(cur[i + 1] - (u8)'0')) <= 9)) \
        sig = num + sig * 10; \
    else { goto digi_stop_##i; }
    REPEAT_INCR_IN_1_18(expr_frac)
#undef expr_frac
    
    cur += 20; /* skip 19 digits and 1 decimal point */
    if (!digi_is_digit(*cur)) goto digi_frac_end; /* fraction part end */
    goto digi_frac_more; /* read more digits in fraction part */
    
    
    /* significant part end */
#define expr_stop(i) \
    digi_stop_##i: \
    cur += i + 1; \
    goto digi_frac_end;
    REPEAT_INCR_IN_1_18(expr_stop)
#undef expr_stop
    
    
    /* read more digits in integral part */
digi_intg_more:
    if (digi_is_digit(*cur)) {
        if (!digi_is_digit_or_fp(cur[1])) {
            /* this number is an integer consisting of 20 digits */
            num = (u64)(*cur - '0');
            if ((sig < (U64_MAX / 10)) ||
                (sig == (U64_MAX / 10) && num <= (U64_MAX % 10))) {
                sig = num + sig * 10;
                cur++;
                /* convert to double if overflow */
                if (sign) {
                    // if (has_read_flag(BIGNUM_AS_RAW)) return_raw();
                    return_f64(normalized_u64_to_f64(sig));
                }
                return_i64(sig);
            }
        }
    }
    
    if (digi_is_exp(*cur)) {
        dot_pos = cur;
        goto digi_exp_more;
    }
    
    if (*cur == '.') {
        dot_pos = cur++;
        if (!digi_is_digit(*cur)) {
            return_err(cur, "no digit after decimal point");
        }
    }
    
    
    /* read more digits in fraction part */
digi_frac_more:
    sig_cut = cur; /* too large to fit in u64, excess digits need to be cut */
    sig += (*cur >= '5'); /* round */
    while (digi_is_digit(*++cur));
    if (!dot_pos) {
        // if (!digi_is_fp(*cur) && has_read_flag(BIGNUM_AS_RAW)) {
        //     return_raw(); /* it's a large integer */
        // }
        dot_pos = cur;
        if (*cur == '.') {
            if (!digi_is_digit(*++cur)) {
                return_err(cur, "no digit after decimal point");
            }
            while (digi_is_digit(*cur)) cur++;
        }
    }
    exp_sig = (i64)(dot_pos - sig_cut);
    exp_sig += (dot_pos < sig_cut);
    
    /* ignore trailing zeros */
    tmp = cur - 1;
    while (*tmp == '0' || *tmp == '.') tmp--;
    if (tmp < sig_cut) {
        sig_cut = NULL;
    } else {
        sig_end = cur;
    }
    
    if (digi_is_exp(*cur)) goto digi_exp_more;
    goto digi_exp_finish;
    
    
    /* fraction part end */
digi_frac_end:
    if (unlikely(dot_pos + 1 == cur)) {
        return_err(cur, "no digit after decimal point");
    }
    sig_end = cur;
    exp_sig = -(i64)((u64)(cur - dot_pos) - 1);
    if (likely(!digi_is_exp(*cur))) {
        if (unlikely(exp_sig < F64_MIN_DEC_EXP - 19)) {
            return_f64_bin(0); /* underflow */
        }
        exp = (i32)exp_sig;
        goto digi_finish;
    } else {
        goto digi_exp_more;
    }
    
    
    /* read exponent part */
digi_exp_more:
    exp_sign = (*++cur == '-');
    cur += digi_is_sign(*cur);
    if (unlikely(!digi_is_digit(*cur))) {
        return_err(cur, "no digit after exponent sign");
    }
    while (*cur == '0') cur++;
    
    /* read exponent literal */
    tmp = cur;
    while (digi_is_digit(*cur)) {
        exp_lit = (i64)((u8)(*cur++ - '0') + (u64)exp_lit * 10);
    }
    if (unlikely(cur - tmp >= U64_SAFE_DIG)) {
        if (exp_sign) {
            return_f64_bin(0); /* underflow */
        } else {
            return_inf(); /* overflow */
        }
    }
    exp_sig += exp_sign ? -exp_lit : exp_lit;
    
    
    /* validate exponent value */
digi_exp_finish:
    if (unlikely(exp_sig < F64_MIN_DEC_EXP - 19)) {
        return_f64_bin(0); /* underflow */
    }
    if (unlikely(exp_sig > F64_MAX_DEC_EXP)) {
        return_inf(); /* overflow */
    }
    exp = (i32)exp_sig;
    
    
    /* all digit read finished */
digi_finish:
    
    /*
     Fast path 1:
     
     1. The floating-point number calculation should be accurate, see the
        comments of macro `PYYJSON_DOUBLE_MATH_CORRECT`.
     2. Correct rounding should be performed (fegetround() == FE_TONEAREST).
     3. The input of floating point number calculation does not lose precision,
        which means: 64 - leading_zero(input) - trailing_zero(input) < 53.
    
     We don't check all available inputs here, because that would make the code
     more complicated, and not friendly to branch predictor.
     */
#if PYYJSON_DOUBLE_MATH_CORRECT
    if (sig < ((u64)1 << 53) &&
        exp >= -F64_POW10_EXP_MAX_EXACT &&
        exp <= +F64_POW10_EXP_MAX_EXACT) {
        f64 dbl = (f64)sig;
        if (exp < 0) {
            dbl /= f64_pow10_table[-exp];
        } else {
            dbl *= f64_pow10_table[+exp];
        }
        return_f64(dbl);
    }
#endif
    
    /*
     Fast path 2:
     
     To keep it simple, we only accept normal number here,
     let the slow path to handle subnormal and infinity number.
     */
    if (likely(!sig_cut &&
               exp > -F64_MAX_DEC_EXP + 1 &&
               exp < +F64_MAX_DEC_EXP - 20)) {
        /*
         The result value is exactly equal to (sig * 10^exp),
         the exponent part (10^exp) can be converted to (sig2 * 2^exp2).
         
         The sig2 can be an infinite length number, only the highest 128 bits
         is cached in the pow10_sig_table.
         
         Now we have these bits:
         sig1 (normalized 64bit)        : aaaaaaaa
         sig2 (higher 64bit)            : bbbbbbbb
         sig2_ext (lower 64bit)         : cccccccc
         sig2_cut (extra unknown bits)  : dddddddddddd....
         
         And the calculation process is:
         ----------------------------------------
                 aaaaaaaa *
                 bbbbbbbbccccccccdddddddddddd....
         ----------------------------------------
         abababababababab +
                 acacacacacacacac +
                         adadadadadadadadadad....
         ----------------------------------------
         [hi____][lo____] +
                 [hi2___][lo2___] +
                         [unknown___________....]
         ----------------------------------------
         
         The addition with carry may affect higher bits, but if there is a 0
         in higher bits, the bits higher than 0 will not be affected.
         
         `lo2` + `unknown` may get a carry bit and may affect `hi2`, the max
         value of `hi2` is 0xFFFFFFFFFFFFFFFE, so `hi2` will not overflow.
         
         `lo` + `hi2` may also get a carry bit and may affect `hi`, but only
         the highest significant 53 bits of `hi` is needed. If there is a 0
         in the lower bits of `hi`, then all the following bits can be dropped.
         
         To convert the result to IEEE-754 double number, we need to perform
         correct rounding:
         1. if bit 54 is 0, round down,
         2. if bit 54 is 1 and any bit beyond bit 54 is 1, round up,
         3. if bit 54 is 1 and all bits beyond bit 54 are 0, round to even,
            as the extra bits is unknown, this case will not be handled here.
         */
        
        u64 raw;
        u64 sig1, sig2, sig2_ext, hi, lo, hi2, lo2, add, bits;
        i32 exp2;
        u32 lz;
        bool exact = false, carry, round_up;
        
        /* convert (10^exp) to (sig2 * 2^exp2) */
        pow10_table_get_sig(exp, &sig2, &sig2_ext);
        pow10_table_get_exp(exp, &exp2);
        
        /* normalize and multiply */
        lz = u64_lz_bits(sig);
        sig1 = sig << lz;
        exp2 -= (i32)lz;
        u128_mul(sig1, sig2, &hi, &lo);
        
        /*
         The `hi` is in range [0x4000000000000000, 0xFFFFFFFFFFFFFFFE],
         To get normalized value, `hi` should be shifted to the left by 0 or 1.
         
         The highest significant 53 bits is used by IEEE-754 double number,
         and the bit 54 is used to detect rounding direction.
         
         The lowest (64 - 54 - 1) bits is used to check whether it contains 0.
         */
        bits = hi & (((u64)1 << (64 - 54 - 1)) - 1);
        if (bits - 1 < (((u64)1 << (64 - 54 - 1)) - 2)) {
            /*
             (bits != 0 && bits != 0x1FF) => (bits - 1 < 0x1FF - 1)
             The `bits` is not zero, so we don't need to check `round to even`
             case. The `bits` contains bit `0`, so we can drop the extra bits
             after `0`.
             */
            exact = true;
            
        } else {
            /*
             (bits == 0 || bits == 0x1FF)
             The `bits` is filled with all `0` or all `1`, so we need to check
             lower bits with another 64-bit multiplication.
             */
            u128_mul(sig1, sig2_ext, &hi2, &lo2);
            
            add = lo + hi2;
            if (add + 1 > (u64)1) {
                /*
                 (add != 0 && add != U64_MAX) => (add + 1 > 1)
                 The `add` is not zero, so we don't need to check `round to
                 even` case. The `add` contains bit `0`, so we can drop the
                 extra bits after `0`. The `hi` cannot be U64_MAX, so it will
                 not overflow.
                 */
                carry = add < lo || add < hi2;
                hi += carry;
                exact = true;
            }
        }
        
        if (exact) {
            /* normalize */
            lz = hi < ((u64)1 << 63);
            hi <<= lz;
            exp2 -= (i32)lz;
            exp2 += 64;
            
            /* test the bit 54 and get rounding direction */
            round_up = (hi & ((u64)1 << (64 - 54))) > (u64)0;
            hi += (round_up ? ((u64)1 << (64 - 54)) : (u64)0);
            
            /* test overflow */
            if (hi < ((u64)1 << (64 - 54))) {
                hi = ((u64)1 << 63);
                exp2 += 1;
            }
            
            /* This is a normal number, convert it to IEEE-754 format. */
            hi >>= F64_BITS - F64_SIG_FULL_BITS;
            exp2 += F64_BITS - F64_SIG_FULL_BITS + F64_SIG_BITS;
            exp2 += F64_EXP_BIAS;
            raw = ((u64)exp2 << F64_SIG_BITS) | (hi & F64_SIG_MASK);
            return_f64_bin(raw);
        }
    }
    
    /*
     Slow path: read double number exactly with diyfp.
     1. Use cached diyfp to get an approximation value.
     2. Use bigcomp to check the approximation value if needed.
     
     This algorithm refers to google's double-conversion project:
     https://github.com/google/double-conversion
     */
    {
        const i32 ERR_ULP_LOG = 3;
        const i32 ERR_ULP = 1 << ERR_ULP_LOG;
        const i32 ERR_CACHED_POW = ERR_ULP / 2;
        const i32 ERR_MUL_FIXED = ERR_ULP / 2;
        const i32 DIY_SIG_BITS = 64;
        const i32 EXP_BIAS = F64_EXP_BIAS + F64_SIG_BITS;
        const i32 EXP_SUBNORMAL = -EXP_BIAS + 1;
        
        u64 fp_err;
        u32 bits;
        i32 order_of_magnitude;
        i32 effective_significand_size;
        i32 precision_digits_count;
        u64 precision_bits;
        u64 half_way;
        
        u64 raw;
        diy_fp fp, fp_upper;
        bigint big_full, big_comp;
        i32 cmp;
        
        fp.sig = sig;
        fp.exp = 0;
        fp_err = sig_cut ? (u64)(ERR_ULP / 2) : (u64)0;
        
        /* normalize */
        bits = u64_lz_bits(fp.sig);
        fp.sig <<= bits;
        fp.exp -= (i32)bits;
        fp_err <<= bits;
        
        /* multiply and add error */
        fp = diy_fp_mul(fp, diy_fp_get_cached_pow10(exp));
        fp_err += (u64)ERR_CACHED_POW + (fp_err != 0) + (u64)ERR_MUL_FIXED;
        
        /* normalize */
        bits = u64_lz_bits(fp.sig);
        fp.sig <<= bits;
        fp.exp -= (i32)bits;
        fp_err <<= bits;
        
        /* effective significand */
        order_of_magnitude = DIY_SIG_BITS + fp.exp;
        if (likely(order_of_magnitude >= EXP_SUBNORMAL + F64_SIG_FULL_BITS)) {
            effective_significand_size = F64_SIG_FULL_BITS;
        } else if (order_of_magnitude <= EXP_SUBNORMAL) {
            effective_significand_size = 0;
        } else {
            effective_significand_size = order_of_magnitude - EXP_SUBNORMAL;
        }
        
        /* precision digits count */
        precision_digits_count = DIY_SIG_BITS - effective_significand_size;
        if (unlikely(precision_digits_count + ERR_ULP_LOG >= DIY_SIG_BITS)) {
            i32 shr = (precision_digits_count + ERR_ULP_LOG) - DIY_SIG_BITS + 1;
            fp.sig >>= shr;
            fp.exp += shr;
            fp_err = (fp_err >> shr) + 1 + (u32)ERR_ULP;
            precision_digits_count -= shr;
        }
        
        /* half way */
        precision_bits = fp.sig & (((u64)1 << precision_digits_count) - 1);
        precision_bits *= (u32)ERR_ULP;
        half_way = (u64)1 << (precision_digits_count - 1);
        half_way *= (u32)ERR_ULP;
        
        /* rounding */
        fp.sig >>= precision_digits_count;
        fp.sig += (precision_bits >= half_way + fp_err);
        fp.exp += precision_digits_count;
        
        /* get IEEE double raw value */
        raw = diy_fp_to_ieee_raw(fp);
        if (unlikely(raw == F64_RAW_INF)) return_inf();
        if (likely(precision_bits <= half_way - fp_err ||
                   precision_bits >= half_way + fp_err)) {
            return_f64_bin(raw); /* number is accurate */
        }
        /* now the number is the correct value, or the next lower value */
        
        /* upper boundary */
        if (raw & F64_EXP_MASK) {
            fp_upper.sig = (raw & F64_SIG_MASK) + ((u64)1 << F64_SIG_BITS);
            fp_upper.exp = (i32)((raw & F64_EXP_MASK) >> F64_SIG_BITS);
        } else {
            fp_upper.sig = (raw & F64_SIG_MASK);
            fp_upper.exp = 1;
        }
        fp_upper.exp -= F64_EXP_BIAS + F64_SIG_BITS;
        fp_upper.sig <<= 1;
        fp_upper.exp -= 1;
        fp_upper.sig += 1; /* add half ulp */
        
        /* compare with bigint */
        bigint_set_buf(&big_full, sig, &exp, sig_cut, sig_end, dot_pos);
        bigint_set_u64(&big_comp, fp_upper.sig);
        if (exp >= 0) {
            bigint_mul_pow10(&big_full, +exp);
        } else {
            bigint_mul_pow10(&big_comp, -exp);
        }
        if (fp_upper.exp > 0) {
            bigint_mul_pow2(&big_comp, (u32)+fp_upper.exp);
        } else {
            bigint_mul_pow2(&big_full, (u32)-fp_upper.exp);
        }
        cmp = bigint_cmp(&big_full, &big_comp);
        if (likely(cmp != 0)) {
            /* round down or round up */
            raw += (cmp > 0);
        } else {
            /* falls midway, round to even */
            raw += (raw & 1);
        }
        
        if (unlikely(raw == F64_RAW_INF)) return_inf();
        return_f64_bin(raw);
    }
    
#undef return_err
#undef return_inf
#undef return_0
#undef return_i64
#undef return_f64
#undef return_f64_bin
#undef return_raw
}



#else /* FP_READER */

/**
 Read a JSON number.
 This is a fallback function if the custom number reader is disabled.
 This function use libc's strtod() to read floating-point number.
 */
force_inline PyObject* read_number(u8 **ptr) {

#define return_err(_end, _msg)                                                  \
    do {                                                                        \
        PyErr_Format(JSONDecodeError, "%s, at position %zu", _msg, _end - hdr); \
        return NULL;                                                           \
    } while (0)

#define return_0() do { \
    *end = cur; \
    return PyLong_FromLongLong(0); \
} while (false)

#define return_i64(_v) do { \
    *end = cur; \
    return PyLong_FromLongLong((i64)(sign ? (u64)(~(_v) + 1) : (u64)(_v))); \
} while (false)

#define return_f64(_v) do { \
    *end = cur; \
    return PyFloat_FromDouble(sign ? -(f64)(_v) : (f64)(_v)); \
} while (false)

#define return_f64_bin(_v) do { \
    *end = cur; \
    u64 temp = ((u64)sign << 63) | (u64)(_v); \
    return PyFloat_FromDouble(*(double *)&temp); \
} while (false)

#define return_inf() do { \
    return_f64_bin(F64_RAW_INF); \
} while (false)

    u64 sig, num;
    u8 *hdr = *ptr;
    u8 *cur = *ptr;
    u8 **end = ptr;
    u8 *dot = NULL;
    u8 *f64_end = NULL;
    bool sign;
    
    sign = (*hdr == '-');
    cur += sign;
    sig = (u8)(*cur - '0');
    
    /* read first digit, check leading zero */
    if (unlikely(!digi_is_digit(*cur))) {
        // if (has_read_flag(ALLOW_INF_AND_NAN)) {
        PyObject* number_obj = read_inf_or_nan(sign, &cur);
        if (likely(number_obj)) {
            *end = cur;
            return number_obj;
        }
        // }
        if(unlikely(!PyErr_Occurred())){
            return_err(cur, "no digit after minus sign");
        }
        return NULL;
    }
    if (*cur == '0') {
        cur++;
        if (unlikely(digi_is_digit(*cur))) {
            return_err(cur - 1, "number with leading zero is not allowed");
        }
        if (!digi_is_fp(*cur)) return_0();
        goto read_double;
    }
    
    /* read continuous digits, up to 19 characters */
#define expr_intg(i) \
    if (likely((num = (u64)(cur[i] - (u8)'0')) <= 9)) sig = num + sig * 10; \
    else { cur += i; goto intg_end; }
    REPEAT_INCR_IN_1_18(expr_intg)
#undef expr_intg
    
    /* here are 19 continuous digits, skip them */
    cur += 19;
    if (digi_is_digit(cur[0]) && !digi_is_digit_or_fp(cur[1])) {
        /* this number is an integer consisting of 20 digits */
        num = (u8)(*cur - '0');
        if ((sig < (U64_MAX / 10)) ||
            (sig == (U64_MAX / 10) && num <= (U64_MAX % 10))) {
            sig = num + sig * 10;
            cur++;
            if (sign) {
                // if (has_read_flag(BIGNUM_AS_RAW)) return_raw();
                return_f64(normalized_u64_to_f64(sig));
            }
            return_i64(sig);
        }
    }
    
intg_end:
    /* continuous digits ended */
    if (!digi_is_digit_or_fp(*cur)) {
        /* this number is an integer consisting of 1 to 19 digits */
        if (sign && (sig > ((u64)1 << 63))) {
            // if (has_read_flag(BIGNUM_AS_RAW)) return_raw();
            return_f64(normalized_u64_to_f64(sig));
        }
        return_i64(sig);
    }
    
read_double:
    /* this number should be read as double */
    while (digi_is_digit(*cur)) cur++;
    // if (!digi_is_fp(*cur) && has_read_flag(BIGNUM_AS_RAW)) {
    //     return_raw(); /* it's a large integer */
    // }
    if (*cur == '.') {
        /* skip fraction part */
        dot = cur;
        cur++;
        if (!digi_is_digit(*cur)) {
            return_err(cur, "no digit after decimal point");
        }
        cur++;
        while (digi_is_digit(*cur)) cur++;
    }
    if (digi_is_exp(*cur)) {
        /* skip exponent part */
        cur += 1 + digi_is_sign(cur[1]);
        if (!digi_is_digit(*cur)) {
            return_err(cur, "no digit after exponent sign");
        }
        cur++;
        while (digi_is_digit(*cur)) cur++;
    }
    
    /*
     libc's strtod() is used to parse the floating-point number.
     
     Note that the decimal point character used by strtod() is locale-dependent,
     and the rounding direction may affected by fesetround().
     
     For currently known locales, (en, zh, ja, ko, am, he, hi) use '.' as the
     decimal point, while other locales use ',' as the decimal point.
     
     Here strtod() is called twice for different locales, but if another thread
     happens calls setlocale() between two strtod(), parsing may still fail.
     */
    // pyyjson_number_op* op_float_final = (pyyjson_number_op*)*op;
    double f = strtod((const char *)hdr, (char **)&f64_end);
    if (unlikely(f64_end != cur)) {
        /* replace '.' with ',' for locale */
        bool cut = (*cur == ',');
        if (cut) *cur = ' ';
        if (dot) *dot = ',';
        // op_float_final->data.
        f = strtod((const char *)hdr, (char **)&f64_end);
        /* restore ',' to '.' */
        if (cut) *cur = ',';
        if (dot) *dot = '.';
        if (unlikely(f64_end != cur)) {
            return_err(hdr, "strtod() failed to parse the number");
        }
    }
    if (unlikely(f >= HUGE_VAL || f <= -HUGE_VAL)) {
        return_inf();
    }

    *end = cur;
    return PyFloat_FromDouble(f);
    
#undef return_err
#undef return_0
#undef return_i64
#undef return_f64
#undef return_f64_bin
#undef return_inf
#undef return_raw
}

#endif /* FP_READER */
