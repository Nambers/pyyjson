#ifndef PYYJSON_ENCODE_FLOAT_H
#define PYYJSON_ENCODE_FLOAT_H

#include "encode_shared.hpp"

/* These codes are modified from yyjson. */

/* IEEE 754 floating-point binary representation */
#ifndef HAS_IEEE_754
#if defined(__STDC_IEC_559__) || defined(__STDC_IEC_60559_BFP__)
#   define HAS_IEEE_754 1
#elif (FLT_RADIX == 2) && (DBL_MANT_DIG == 53) && (DBL_DIG == 15) && \
     (DBL_MIN_EXP == -1021) && (DBL_MAX_EXP == 1024) && \
     (DBL_MIN_10_EXP == -307) && (DBL_MAX_10_EXP == 308)
#   define HAS_IEEE_754 1
#else
#   define HAS_IEEE_754 0
#endif
#endif


/** Minimum decimal exponent in pow10_sig_table. */
#define POW10_SIG_TABLE_MIN_EXP -343

/** Maximum decimal exponent in pow10_sig_table. */
#define POW10_SIG_TABLE_MAX_EXP 324

/** Minimum exact decimal exponent in pow10_sig_table */
#define POW10_SIG_TABLE_MIN_EXACT_EXP 0

/** Maximum exact decimal exponent in pow10_sig_table */
#define POW10_SIG_TABLE_MAX_EXACT_EXP 55


/*==============================================================================
 * Float Writer
 *============================================================================*/

#if HAS_IEEE_754

/** Trailing zero count table for number 0 to 99.
    (generate with misc/make_tables.c) */
static const u8 dec_trailing_zero_table[] = {
    2, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

/** Write an unsigned integer with a length of 1 to 16. */
static_inline u8 *write_u64_len_1_to_16(u64 val, u8 *buf) {
    u64 hgh;
    u32 low;
    if (val < 100000000) {                          /* 1-8 digits */
        buf = write_u32_len_1_8((u32)val, buf);
        return buf;
    } else {                                        /* 9-16 digits */
        hgh = val / 100000000;                      /* (val / 100000000) */
        low = (u32)(val - hgh * 100000000);         /* (val % 100000000) */
        buf = write_u32_len_1_8((u32)hgh, buf);
        buf = write_u32_len_8(low, buf);
        return buf;
    }
}

/** Write an unsigned integer with a length of 1 to 17. */
static_inline u8 *write_u64_len_1_to_17(u64 val, u8 *buf) {
    u64 hgh;
    u32 mid, low, one;
    if (val >= (u64)100000000 * 10000000) {         /* len: 16 to 17 */
        hgh = val / 100000000;                      /* (val / 100000000) */
        low = (u32)(val - hgh * 100000000);         /* (val % 100000000) */
        one = (u32)(hgh / 100000000);               /* (hgh / 100000000) */
        mid = (u32)(hgh - (u64)one * 100000000);    /* (hgh % 100000000) */
        *buf = (u8)((u8)one + (u8)'0');
        buf += one > 0;
        buf = write_u32_len_8(mid, buf);
        buf = write_u32_len_8(low, buf);
        return buf;
    } else if (val >= (u64)100000000){              /* len: 9 to 15 */
        hgh = val / 100000000;                      /* (val / 100000000) */
        low = (u32)(val - hgh * 100000000);         /* (val % 100000000) */
        buf = write_u32_len_1_8((u32)hgh, buf);
        buf = write_u32_len_8(low, buf);
        return buf;
    } else { /* len: 1 to 8 */
        buf = write_u32_len_1_8((u32)val, buf);
        return buf;
    }
}

/**
 Write an unsigned integer with a length of 15 to 17 with trailing zero trimmed.
 These digits are named as "aabbccddeeffgghhii" here.
 For example, input 1234567890123000, output "1234567890123".
 */
static_inline u8 *write_u64_len_15_to_17_trim(u8 *buf, u64 sig) {
    bool lz;                                        /* leading zero */
    u32 tz1, tz2, tz;                               /* trailing zero */
    
    u32 abbccddee = (u32)(sig / 100000000);
    u32 ffgghhii = (u32)(sig - (u64)abbccddee * 100000000);
    u32 abbcc = abbccddee / 10000;                  /* (abbccddee / 10000) */
    u32 ddee = abbccddee - abbcc * 10000;           /* (abbccddee % 10000) */
    u32 abb = (u32)(((u64)abbcc * 167773) >> 24);   /* (abbcc / 100) */
    u32 a = (abb * 41) >> 12;                       /* (abb / 100) */
    u32 bb = abb - a * 100;                         /* (abb % 100) */
    u32 cc = abbcc - abb * 100;                     /* (abbcc % 100) */
    
    /* write abbcc */
    buf[0] = (u8)(a + '0');
    buf += a > 0;
    lz = bb < 10 && a == 0;
    byte_copy_2(buf + 0, digit_table + bb * 2 + lz);
    buf -= lz;
    byte_copy_2(buf + 2, digit_table + cc * 2);
    
    if (ffgghhii) {
        u32 dd = (ddee * 5243) >> 19;               /* (ddee / 100) */
        u32 ee = ddee - dd * 100;                   /* (ddee % 100) */
        u32 ffgg = (u32)(((u64)ffgghhii * 109951163) >> 40); /* (val / 10000) */
        u32 hhii = ffgghhii - ffgg * 10000;         /* (val % 10000) */
        u32 ff = (ffgg * 5243) >> 19;               /* (aabb / 100) */
        u32 gg = ffgg - ff * 100;                   /* (aabb % 100) */
        byte_copy_2(buf + 4, digit_table + dd * 2);
        byte_copy_2(buf + 6, digit_table + ee * 2);
        byte_copy_2(buf + 8, digit_table + ff * 2);
        byte_copy_2(buf + 10, digit_table + gg * 2);
        if (hhii) {
            u32 hh = (hhii * 5243) >> 19;           /* (ccdd / 100) */
            u32 ii = hhii - hh * 100;               /* (ccdd % 100) */
            byte_copy_2(buf + 12, digit_table + hh * 2);
            byte_copy_2(buf + 14, digit_table + ii * 2);
            tz1 = dec_trailing_zero_table[hh];
            tz2 = dec_trailing_zero_table[ii];
            tz = ii ? tz2 : (tz1 + 2);
            buf += 16 - tz;
            return buf;
        } else {
            tz1 = dec_trailing_zero_table[ff];
            tz2 = dec_trailing_zero_table[gg];
            tz = gg ? tz2 : (tz1 + 2);
            buf += 12 - tz;
            return buf;
        }
    } else {
        if (ddee) {
            u32 dd = (ddee * 5243) >> 19;           /* (ddee / 100) */
            u32 ee = ddee - dd * 100;               /* (ddee % 100) */
            byte_copy_2(buf + 4, digit_table + dd * 2);
            byte_copy_2(buf + 6, digit_table + ee * 2);
            tz1 = dec_trailing_zero_table[dd];
            tz2 = dec_trailing_zero_table[ee];
            tz = ee ? tz2 : (tz1 + 2);
            buf += 8 - tz;
            return buf;
        } else {
            tz1 = dec_trailing_zero_table[bb];
            tz2 = dec_trailing_zero_table[cc];
            tz = cc ? tz2 : (tz1 + tz2);
            buf += 4 - tz;
            return buf;
        }
    }
}

/** Write a signed integer in the range -324 to 308. */
static_inline u8 *write_f64_exp(i32 exp, u8 *buf) {
    buf[0] = '-';
    buf += exp < 0;
    exp = exp < 0 ? -exp : exp;
    if (exp < 100) {
        u32 lz = exp < 10;
        byte_copy_2(buf + 0, digit_table + (u32)exp * 2 + lz);
        return buf + 2 - lz;
    } else {
        u32 hi = ((u32)exp * 656) >> 16;            /* exp / 100 */
        u32 lo = (u32)exp - hi * 100;               /* exp % 100 */
        buf[0] = (u8)((u8)hi + (u8)'0');
        byte_copy_2(buf + 1, digit_table + lo * 2);
        return buf + 3;
    }
}

/** Multiplies 128-bit integer and returns highest 64-bit rounded value. */
static_inline u64 round_to_odd(u64 hi, u64 lo, u64 cp) {
    u64 x_hi, x_lo, y_hi, y_lo;
    u128_mul(cp, lo, &x_hi, &x_lo);
    u128_mul_add(cp, hi, x_hi, &y_hi, &y_lo);
    return y_hi | (y_lo > 1);
}

/**
 Convert double number from binary to decimal.
 The output significand is shortest decimal but may have trailing zeros.
 
 This function use the Schubfach algorithm:
 Raffaello Giulietti, The Schubfach way to render doubles (5th version), 2022.
 https://drive.google.com/file/d/1gp5xv4CAa78SVgCeWfGqqI4FfYYYuNFb
 https://mail.openjdk.java.net/pipermail/core-libs-dev/2021-November/083536.html
 https://github.com/openjdk/jdk/pull/3402 (Java implementation)
 https://github.com/abolz/Drachennest (C++ implementation)
 
 See also:
 Dragonbox: A New Floating-Point Binary-to-Decimal Conversion Algorithm, 2022.
 https://github.com/jk-jeon/dragonbox/blob/master/other_files/Dragonbox.pdf
 https://github.com/jk-jeon/dragonbox
 
 @param sig_raw The raw value of significand in IEEE 754 format.
 @param exp_raw The raw value of exponent in IEEE 754 format.
 @param sig_bin The decoded value of significand in binary.
 @param exp_bin The decoded value of exponent in binary.
 @param sig_dec The output value of significand in decimal.
 @param exp_dec The output value of exponent in decimal.
 @warning The input double number should not be 0, inf, nan.
 */
static_inline void f64_bin_to_dec(u64 sig_raw, u32 exp_raw,
                                  u64 sig_bin, i32 exp_bin,
                                  u64 *sig_dec, i32 *exp_dec) {
    
    bool is_even, regular_spacing, u_inside, w_inside, round_up;
    u64 s, sp, cb, cbl, cbr, vb, vbl, vbr, pow10hi, pow10lo, upper, lower, mid;
    i32 k, h, exp10;
    
    is_even = !(sig_bin & 1);
    regular_spacing = (sig_raw == 0 && exp_raw > 1);
    
    cbl = 4 * sig_bin - 2 + regular_spacing;
    cb  = 4 * sig_bin;
    cbr = 4 * sig_bin + 2;
    
    /* exp_bin: [-1074, 971]                                                  */
    /* k = regular_spacing ? floor(log10(pow(2, exp_bin)))                    */
    /*                     : floor(log10(pow(2, exp_bin) * 3.0 / 4.0))        */
    /*   = regular_spacing ? floor(exp_bin * log10(2))                        */
    /*                     : floor(exp_bin * log10(2) + log10(3.0 / 4.0))     */
    k = (i32)(exp_bin * 315653 - (regular_spacing ? 131237 : 0)) >> 20;
    
    /* k: [-324, 292]                                                         */
    /* h = exp_bin + floor(log2(pow(10, e)))                                  */
    /*   = exp_bin + floor(log2(10) * e)                                      */
    exp10 = -k;
    h = exp_bin + ((exp10 * 217707) >> 16) + 1;
    
    pow10_table_get_sig(exp10, &pow10hi, &pow10lo);
    pow10lo += (exp10 < POW10_SIG_TABLE_MIN_EXACT_EXP ||
                exp10 > POW10_SIG_TABLE_MAX_EXACT_EXP);
    vbl = round_to_odd(pow10hi, pow10lo, cbl << h);
    vb  = round_to_odd(pow10hi, pow10lo, cb  << h);
    vbr = round_to_odd(pow10hi, pow10lo, cbr << h);
    
    lower = vbl + !is_even;
    upper = vbr - !is_even;
    
    s = vb / 4;
    if (s >= 10) {
        sp = s / 10;
        u_inside = (lower <= 40 * sp);
        w_inside = (upper >= 40 * sp + 40);
        if (u_inside != w_inside) {
            *sig_dec = sp + w_inside;
            *exp_dec = k + 1;
            return;
        }
    }
    
    u_inside = (lower <= 4 * s);
    w_inside = (upper >= 4 * s + 4);
    
    mid = 4 * s + 2;
    round_up = (vb > mid) || (vb == mid && (s & 1) != 0);
    
    *sig_dec = s + ((u_inside != w_inside) ? w_inside : round_up);
    *exp_dec = k;
}

/**
 Write a double number (requires 32 bytes buffer).
 
 We follows the ECMAScript specification to print floating point numbers,
 but with the following changes:
 1. Keep the negative sign of 0.0 to preserve input information.
 2. Keep decimal point to indicate the number is floating point.
 3. Remove positive sign of exponent part.
 */
static_inline u8 *write_f64_raw(u8 *buf, u64 raw) {
    u64 sig_bin, sig_dec, sig_raw;
    i32 exp_bin, exp_dec, sig_len, dot_pos, i, max;
    u32 exp_raw, hi, lo;
    u8 *hdr, *num_hdr, *num_end, *dot_end;
    bool sign;
    
    /* decode raw bytes from IEEE-754 double format. */
    sign = (bool)(raw >> (F64_BITS - 1));
    sig_raw = raw & F64_SIG_MASK;
    exp_raw = (u32)((raw & F64_EXP_MASK) >> F64_SIG_BITS);
    
    /* return inf and nan */
    if (unlikely(exp_raw == ((u32)1 << F64_EXP_BITS) - 1)) {
        if (sig_raw == 0) {
            buf[0] = '-';
            buf += sign;
            byte_copy_8(buf, "Infinity");
            buf += 8;
            return buf;
        } else {
            byte_copy_4(buf, "NaN");
            return buf + 3;
        }
        return NULL;
    }
    
    /* add sign for all finite double value, including 0.0 and inf */
    buf[0] = '-';
    buf += sign;
    hdr = buf;
    
    /* return zero */
    if ((raw << 1) == 0) {
        byte_copy_4(buf, "0.0");
        buf += 3;
        return buf;
    }
    
    if (likely(exp_raw != 0)) {
        /* normal number */
        sig_bin = sig_raw | ((u64)1 << F64_SIG_BITS);
        exp_bin = (i32)exp_raw - F64_EXP_BIAS - F64_SIG_BITS;
        
        /* fast path for small integer number without fraction */
        if (-F64_SIG_BITS <= exp_bin && exp_bin <= 0) {
            if (u64_tz_bits(sig_bin) >= (u32)-exp_bin) {
                /* number is integer in range 1 to 0x1FFFFFFFFFFFFF */
                sig_dec = sig_bin >> -exp_bin;
                buf = write_u64_len_1_to_16(sig_dec, buf);
                byte_copy_2(buf, ".0");
                buf += 2;
                return buf;
            }
        }
        
        /* binary to decimal */
        f64_bin_to_dec(sig_raw, exp_raw, sig_bin, exp_bin, &sig_dec, &exp_dec);
        
        /* the sig length is 15 to 17 */
        sig_len = 17;
        sig_len -= (sig_dec < (u64)100000000 * 100000000);
        sig_len -= (sig_dec < (u64)100000000 * 10000000);
        
        /* the decimal point position relative to the first digit */
        dot_pos = sig_len + exp_dec;
        
        if (-6 < dot_pos && dot_pos <= 21) {
            /* no need to write exponent part */
            if (dot_pos <= 0) {
                /* dot before first digit */
                /* such as 0.1234, 0.000001234 */
                num_hdr = hdr + (2 - dot_pos);
                num_end = write_u64_len_15_to_17_trim(num_hdr, sig_dec);
                hdr[0] = '0';
                hdr[1] = '.';
                hdr += 2;
                max = -dot_pos;
                for (i = 0; i < max; i++) hdr[i] = '0';
                return num_end;
            } else {
                /* dot after first digit */
                /* such as 1.234, 1234.0, 123400000000000000000.0 */
                memset(hdr +  0, '0', 8);
                memset(hdr +  8, '0', 8);
                memset(hdr + 16, '0', 8);
                num_hdr = hdr + 1;
                num_end = write_u64_len_15_to_17_trim(num_hdr, sig_dec);
                for (i = 0; i < dot_pos; i++) hdr[i] = hdr[i + 1];
                hdr[dot_pos] = '.';
                dot_end = hdr + dot_pos + 2;
                return dot_end < num_end ? num_end : dot_end;
            }
        } else {
            /* write with scientific notation */
            /* such as 1.234e56 */
            u8 *end = write_u64_len_15_to_17_trim(buf + 1, sig_dec);
            end -= (end == buf + 2); /* remove '.0', e.g. 2.0e34 -> 2e34 */
            exp_dec += sig_len - 1;
            hdr[0] = hdr[1];
            hdr[1] = '.';
            end[0] = 'e';
            buf = write_f64_exp(exp_dec, end + 1);
            return buf;
        }
        
    } else {
        /* subnormal number */
        sig_bin = sig_raw;
        exp_bin = 1 - F64_EXP_BIAS - F64_SIG_BITS;
        
        /* binary to decimal */
        f64_bin_to_dec(sig_raw, exp_raw, sig_bin, exp_bin, &sig_dec, &exp_dec);
        
        /* write significand part */
        buf = write_u64_len_1_to_17(sig_dec, buf + 1);
        hdr[0] = hdr[1];
        hdr[1] = '.';
        do {
            buf--;
            exp_dec++;
        } while (*buf == '0');
        exp_dec += (i32)(buf - hdr - 2);
        buf += (*buf != '.');
        buf[0] = 'e';
        buf++;
        
        /* write exponent part */
        buf[0] = '-';
        buf++;
        exp_dec = -exp_dec;
        hi = ((u32)exp_dec * 656) >> 16; /* exp / 100 */
        lo = (u32)exp_dec - hi * 100; /* exp % 100 */
        buf[0] = (u8)((u8)hi + (u8)'0');
        byte_copy_2(buf + 1, digit_table + lo * 2);
        buf += 3;
        return buf;
    }
}


#else


/** Write a double number (requires 32 bytes buffer). */
static_inline u8 *write_f64_raw(u8 *buf, u64 raw) {
    /*
     For IEEE 754, `DBL_DECIMAL_DIG` is 17 for round-trip.
     For non-IEEE formats, 17 is used to avoid buffer overflow,
     round-trip is not guaranteed.
     */
#if defined(DBL_DECIMAL_DIG) && DBL_DECIMAL_DIG != 17
    int dig = DBL_DECIMAL_DIG > 17 ? 17 : DBL_DECIMAL_DIG;
#else
    int dig = 17;
#endif
    
    /*
     The snprintf() function is locale-dependent. For currently known locales,
     (en, zh, ja, ko, am, he, hi) use '.' as the decimal point, while other
     locales use ',' as the decimal point. we need to replace ',' with '.'
     to avoid the locale setting.
     */
    f64 val = f64_from_raw(raw);
#if YYJSON_MSC_VER >= 1400
    int len = sprintf_s((char *)buf, 32, "%.*g", dig, val);
#elif defined(snprintf) || (YYJSON_STDC_VER >= 199901L)
    int len = snprintf((char *)buf, 32, "%.*g", dig, val);
#else
    int len = sprintf((char *)buf, "%.*g", dig, val);
#endif
    
    u8 *cur = buf;
    if (unlikely(len < 1)) return NULL;
    cur += (*cur == '-');
    if (unlikely(!digi_is_digit(*cur))) {
        /* nan, inf, or bad output */
        if (*cur == 'i') {
            byte_copy_8(cur, "Infinity");
            cur += 8;
            return cur;
        } else if (*cur == 'n') {
            byte_copy_4(buf, "NaN");
            return buf + 3;
        }
        return NULL;
    } else {
        /* finite number */
        int i = 0;
        bool fp = false;
        for (; i < len; i++) {
            if (buf[i] == ',') buf[i] = '.';
            if (digi_is_fp((u8)buf[i])) fp = true;
        }
        if (!fp) {
            buf[len++] = '.';
            buf[len++] = '0';
        }
    }
    return buf + len;
}

#endif


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

/** Digit table from 00 to 99. */
yyjson_align(2)
static const char digit_table[200] = {
    '0', '0', '0', '1', '0', '2', '0', '3', '0', '4',
    '0', '5', '0', '6', '0', '7', '0', '8', '0', '9',
    '1', '0', '1', '1', '1', '2', '1', '3', '1', '4',
    '1', '5', '1', '6', '1', '7', '1', '8', '1', '9',
    '2', '0', '2', '1', '2', '2', '2', '3', '2', '4',
    '2', '5', '2', '6', '2', '7', '2', '8', '2', '9',
    '3', '0', '3', '1', '3', '2', '3', '3', '3', '4',
    '3', '5', '3', '6', '3', '7', '3', '8', '3', '9',
    '4', '0', '4', '1', '4', '2', '4', '3', '4', '4',
    '4', '5', '4', '6', '4', '7', '4', '8', '4', '9',
    '5', '0', '5', '1', '5', '2', '5', '3', '5', '4',
    '5', '5', '5', '6', '5', '7', '5', '8', '5', '9',
    '6', '0', '6', '1', '6', '2', '6', '3', '6', '4',
    '6', '5', '6', '6', '6', '7', '6', '8', '6', '9',
    '7', '0', '7', '1', '7', '2', '7', '3', '7', '4',
    '7', '5', '7', '6', '7', '7', '7', '8', '7', '9',
    '8', '0', '8', '1', '8', '2', '8', '3', '8', '4',
    '8', '5', '8', '6', '8', '7', '8', '8', '8', '9',
    '9', '0', '9', '1', '9', '2', '9', '3', '9', '4',
    '9', '5', '9', '6', '9', '7', '9', '8', '9', '9'
};

static_inline u8 *write_u32_len_8(u32 val, u8 *buf) {
    u32 aa, bb, cc, dd, aabb, ccdd;                 /* 8 digits: aabbccdd */
    aabb = (u32)(((u64)val * 109951163) >> 40);     /* (val / 10000) */
    ccdd = val - aabb * 10000;                      /* (val % 10000) */
    aa = (aabb * 5243) >> 19;                       /* (aabb / 100) */
    cc = (ccdd * 5243) >> 19;                       /* (ccdd / 100) */
    bb = aabb - aa * 100;                           /* (aabb % 100) */
    dd = ccdd - cc * 100;                           /* (ccdd % 100) */
    byte_copy_2(buf + 0, digit_table + aa * 2);
    byte_copy_2(buf + 2, digit_table + bb * 2);
    byte_copy_2(buf + 4, digit_table + cc * 2);
    byte_copy_2(buf + 6, digit_table + dd * 2);
    return buf + 8;
}

static_inline u8 *write_u32_len_4(u32 val, u8 *buf) {
    u32 aa, bb;                                     /* 4 digits: aabb */
    aa = (val * 5243) >> 19;                        /* (val / 100) */
    bb = val - aa * 100;                            /* (val % 100) */
    byte_copy_2(buf + 0, digit_table + aa * 2);
    byte_copy_2(buf + 2, digit_table + bb * 2);
    return buf + 4;
}

static_inline u8 *write_u32_len_1_8(u32 val, u8 *buf) {
    u32 aa, bb, cc, dd, aabb, bbcc, ccdd, lz;
    
    if (val < 100) {                                /* 1-2 digits: aa */
        lz = val < 10;                              /* leading zero: 0 or 1 */
        byte_copy_2(buf + 0, digit_table + val * 2 + lz);
        buf -= lz;
        return buf + 2;
        
    } else if (val < 10000) {                       /* 3-4 digits: aabb */
        aa = (val * 5243) >> 19;                    /* (val / 100) */
        bb = val - aa * 100;                        /* (val % 100) */
        lz = aa < 10;                               /* leading zero: 0 or 1 */
        byte_copy_2(buf + 0, digit_table + aa * 2 + lz);
        buf -= lz;
        byte_copy_2(buf + 2, digit_table + bb * 2);
        return buf + 4;
        
    } else if (val < 1000000) {                     /* 5-6 digits: aabbcc */
        aa = (u32)(((u64)val * 429497) >> 32);      /* (val / 10000) */
        bbcc = val - aa * 10000;                    /* (val % 10000) */
        bb = (bbcc * 5243) >> 19;                   /* (bbcc / 100) */
        cc = bbcc - bb * 100;                       /* (bbcc % 100) */
        lz = aa < 10;                               /* leading zero: 0 or 1 */
        byte_copy_2(buf + 0, digit_table + aa * 2 + lz);
        buf -= lz;
        byte_copy_2(buf + 2, digit_table + bb * 2);
        byte_copy_2(buf + 4, digit_table + cc * 2);
        return buf + 6;
        
    } else {                                        /* 7-8 digits: aabbccdd */
        aabb = (u32)(((u64)val * 109951163) >> 40); /* (val / 10000) */
        ccdd = val - aabb * 10000;                  /* (val % 10000) */
        aa = (aabb * 5243) >> 19;                   /* (aabb / 100) */
        cc = (ccdd * 5243) >> 19;                   /* (ccdd / 100) */
        bb = aabb - aa * 100;                       /* (aabb % 100) */
        dd = ccdd - cc * 100;                       /* (ccdd % 100) */
        lz = aa < 10;                               /* leading zero: 0 or 1 */
        byte_copy_2(buf + 0, digit_table + aa * 2 + lz);
        buf -= lz;
        byte_copy_2(buf + 2, digit_table + bb * 2);
        byte_copy_2(buf + 4, digit_table + cc * 2);
        byte_copy_2(buf + 6, digit_table + dd * 2);
        return buf + 8;
    }
}

static_inline u8 *write_u64_len_5_8(u32 val, u8 *buf) {
    u32 aa, bb, cc, dd, aabb, bbcc, ccdd, lz;
    
    if (val < 1000000) {                            /* 5-6 digits: aabbcc */
        aa = (u32)(((u64)val * 429497) >> 32);      /* (val / 10000) */
        bbcc = val - aa * 10000;                    /* (val % 10000) */
        bb = (bbcc * 5243) >> 19;                   /* (bbcc / 100) */
        cc = bbcc - bb * 100;                       /* (bbcc % 100) */
        lz = aa < 10;                               /* leading zero: 0 or 1 */
        byte_copy_2(buf + 0, digit_table + aa * 2 + lz);
        buf -= lz;
        byte_copy_2(buf + 2, digit_table + bb * 2);
        byte_copy_2(buf + 4, digit_table + cc * 2);
        return buf + 6;
        
    } else {                                        /* 7-8 digits: aabbccdd */
        aabb = (u32)(((u64)val * 109951163) >> 40); /* (val / 10000) */
        ccdd = val - aabb * 10000;                  /* (val % 10000) */
        aa = (aabb * 5243) >> 19;                   /* (aabb / 100) */
        cc = (ccdd * 5243) >> 19;                   /* (ccdd / 100) */
        bb = aabb - aa * 100;                       /* (aabb % 100) */
        dd = ccdd - cc * 100;                       /* (ccdd % 100) */
        lz = aa < 10;                               /* leading zero: 0 or 1 */
        byte_copy_2(buf + 0, digit_table + aa * 2 + lz);
        buf -= lz;
        byte_copy_2(buf + 2, digit_table + bb * 2);
        byte_copy_2(buf + 4, digit_table + cc * 2);
        byte_copy_2(buf + 6, digit_table + dd * 2);
        return buf + 8;
    }
}

static_inline u8 *write_u64(u64 val, u8 *buf) {
    u64 tmp, hgh;
    u32 mid, low;
    
    if (val < 100000000) {                          /* 1-8 digits */
        buf = write_u32_len_1_8((u32)val, buf);
        return buf;
        
    } else if (val < (u64)100000000 * 100000000) {  /* 9-16 digits */
        hgh = val / 100000000;                      /* (val / 100000000) */
        low = (u32)(val - hgh * 100000000);         /* (val % 100000000) */
        buf = write_u32_len_1_8((u32)hgh, buf);
        buf = write_u32_len_8(low, buf);
        return buf;
        
    } else {                                        /* 17-20 digits */
        tmp = val / 100000000;                      /* (val / 100000000) */
        low = (u32)(val - tmp * 100000000);         /* (val % 100000000) */
        hgh = (u32)(tmp / 10000);                   /* (tmp / 10000) */
        mid = (u32)(tmp - hgh * 10000);             /* (tmp % 10000) */
        buf = write_u64_len_5_8((u32)hgh, buf);
        buf = write_u32_len_4(mid, buf);
        buf = write_u32_len_8(low, buf);
        return buf;
    }
}




#endif // PYYJSON_ENCODE_FLOAT_H