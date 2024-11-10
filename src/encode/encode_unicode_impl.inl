#include "simd_detect.h"
#include "simd_impl.h"
#include <immintrin.h>

#ifndef COMPILE_READ_UCS_LEVEL
#error "COMPILE_READ_UCS_LEVEL is not defined"
#endif

#ifndef COMPILE_WRITE_UCS_LEVEL
#error "COMPILE_WRITE_UCS_LEVEL is not defined"
#endif

#ifndef COMPILE_INDENT_LEVEL
#error "COMPILE_INDENT_LEVEL is not defined"
#endif

#if COMPILE_WRITE_UCS_LEVEL == 4
#define _WRITER U32_WRITER
#define _TARGET_TYPE u32
#elif COMPILE_WRITE_UCS_LEVEL == 2
#define _WRITER U16_WRITER
#define _TARGET_TYPE u16
#elif COMPILE_WRITE_UCS_LEVEL == 1
#define _WRITER U8_WRITER
#define _TARGET_TYPE u8
#else
#error "COMPILE_WRITE_UCS_LEVEL must be 1, 2 or 4"
#endif

#if COMPILE_READ_UCS_LEVEL == 4
#define CHECK_MASK_TYPE u16
#define _FROM_TYPE u32
#elif COMPILE_READ_UCS_LEVEL == 2
#define CHECK_MASK_TYPE u32
#define _FROM_TYPE u16
#elif COMPILE_READ_UCS_LEVEL == 1
#define CHECK_MASK_TYPE u64
#define _FROM_TYPE u8
#else
#error "COMPILE_READ_UCS_LEVEL must be 1, 2 or 4"
#endif


#define CHECK_COUNT_MAX (SIMD_BIT_SIZE / 8 / sizeof(_FROM_TYPE))

// encode_utils_impl.inl
#define VEC_RESERVE PYYJSON_CONCAT2(vec_reserve, COMPILE_WRITE_UCS_LEVEL)
// encode_simd_utils.inl
#define CHECK_ESCAPE_IMPL_GET_MASK PYYJSON_CONCAT2(check_escape_impl_get_mask, COMPILE_READ_UCS_LEVEL)
#define CHECK_MASK_ZERO PYYJSON_CONCAT2(check_mask_zero, COMPILE_READ_UCS_LEVEL)
#define CHECK_MASK_ZERO_SMALL PYYJSON_CONCAT2(check_mask_zero_small, COMPILE_READ_UCS_LEVEL)
#define WRITE_SIMD_256_WITH_WRITEMASK PYYJSON_CONCAT2(write_simd_256_with_writemask, COMPILE_WRITE_UCS_LEVEL)
#define WRITE_SIMD_WITH_TAIL_LEN PYYJSON_CONCAT3(write_simd_with_tail_len, COMPILE_READ_UCS_LEVEL, COMPILE_WRITE_UCS_LEVEL)
// define here
#define _CONTROL_SEQ_TABLE PYYJSON_CONCAT2(_ControlSeqTable, COMPILE_WRITE_UCS_LEVEL)
// define here
#define VECTOR_WRITE_UNICODE_IMPL PYYJSON_CONCAT4(vector_write_unicode_impl, COMPILE_INDENT_LEVEL, COMPILE_READ_UCS_LEVEL, COMPILE_WRITE_UCS_LEVEL)
#define VECTOR_WRITE_UNICODE_TRAILING_IMPL PYYJSON_CONCAT4(vector_write_unicode_trailing_impl, COMPILE_INDENT_LEVEL, COMPILE_READ_UCS_LEVEL, COMPILE_WRITE_UCS_LEVEL)
#define VECTOR_WRITE_ESCAPE_IMPL PYYJSON_CONCAT4(vector_write_escape_impl, COMPILE_INDENT_LEVEL, COMPILE_READ_UCS_LEVEL, COMPILE_WRITE_UCS_LEVEL)
#define VECTOR_WRITE_INDENT PYYJSON_CONCAT3(vector_write_indent, COMPILE_INDENT_LEVEL, COMPILE_WRITE_UCS_LEVEL)
#define MASK_RIGHT_SHIFT PYYJSON_CONCAT2(mask_right_shift, COMPILE_READ_UCS_LEVEL)
#define SIMD_RIGHT_SHIFT PYYJSON_CONCAT2(simd_right_shift, COMPILE_READ_UCS_LEVEL)
#define CHECK_ESCAPE_TAIL_IMPL_GET_MASK_512 PYYJSON_CONCAT2(check_escape_tail_impl_get_mask_512, COMPILE_READ_UCS_LEVEL)
#define WRITE_SIMD_IMPL PYYJSON_CONCAT3(write_simd_impl, COMPILE_READ_UCS_LEVEL, COMPILE_WRITE_UCS_LEVEL)
#define MASK_ELEVATE_WRITE_512 PYYJSON_CONCAT3(mask_elevate_write_512, COMPILE_READ_UCS_LEVEL, COMPILE_WRITE_UCS_LEVEL)

#if COMPILE_READ_UCS_LEVEL == 1 && COMPILE_INDENT_LEVEL == 0
static _TARGET_TYPE _CONTROL_SEQ_TABLE[(_Slash + 1) * 8] = {
        CONTROL_SEQ_ESCAPE_PREFIX, '0', '0', CONTROL_SEQ_ESCAPE_SUFFIX,  // 0
        CONTROL_SEQ_ESCAPE_PREFIX, '0', '1', CONTROL_SEQ_ESCAPE_SUFFIX,  // 1
        CONTROL_SEQ_ESCAPE_PREFIX, '0', '2', CONTROL_SEQ_ESCAPE_SUFFIX,  // 2
        CONTROL_SEQ_ESCAPE_PREFIX, '0', '3', CONTROL_SEQ_ESCAPE_SUFFIX,  // 3
        CONTROL_SEQ_ESCAPE_PREFIX, '0', '4', CONTROL_SEQ_ESCAPE_SUFFIX,  // 4
        CONTROL_SEQ_ESCAPE_PREFIX, '0', '5', CONTROL_SEQ_ESCAPE_SUFFIX,  // 5
        CONTROL_SEQ_ESCAPE_PREFIX, '0', '6', CONTROL_SEQ_ESCAPE_SUFFIX,  // 6
        CONTROL_SEQ_ESCAPE_PREFIX, '0', '7', CONTROL_SEQ_ESCAPE_SUFFIX,  // 7
        '\\', 'b', CONTROL_SEQ_ESCAPE_MIDDLE, CONTROL_SEQ_ESCAPE_SUFFIX, // 8
        '\\', 't', CONTROL_SEQ_ESCAPE_MIDDLE, CONTROL_SEQ_ESCAPE_SUFFIX, // 9
        '\\', 'n', CONTROL_SEQ_ESCAPE_MIDDLE, CONTROL_SEQ_ESCAPE_SUFFIX, // 10
        CONTROL_SEQ_ESCAPE_PREFIX, '0', 'b', CONTROL_SEQ_ESCAPE_SUFFIX,  // 11
        '\\', 'f', CONTROL_SEQ_ESCAPE_MIDDLE, CONTROL_SEQ_ESCAPE_SUFFIX, // 12
        '\\', 'r', CONTROL_SEQ_ESCAPE_MIDDLE, CONTROL_SEQ_ESCAPE_SUFFIX, // 13
        CONTROL_SEQ_ESCAPE_PREFIX, '0', 'e', CONTROL_SEQ_ESCAPE_SUFFIX,  // 14
        CONTROL_SEQ_ESCAPE_PREFIX, '0', 'f', CONTROL_SEQ_ESCAPE_SUFFIX,  // 15
        CONTROL_SEQ_ESCAPE_PREFIX, '1', '0', CONTROL_SEQ_ESCAPE_SUFFIX,  // 16
        CONTROL_SEQ_ESCAPE_PREFIX, '1', '1', CONTROL_SEQ_ESCAPE_SUFFIX,  // 17
        CONTROL_SEQ_ESCAPE_PREFIX, '1', '2', CONTROL_SEQ_ESCAPE_SUFFIX,  // 18
        CONTROL_SEQ_ESCAPE_PREFIX, '1', '3', CONTROL_SEQ_ESCAPE_SUFFIX,  // 19
        CONTROL_SEQ_ESCAPE_PREFIX, '1', '4', CONTROL_SEQ_ESCAPE_SUFFIX,  // 20
        CONTROL_SEQ_ESCAPE_PREFIX, '1', '5', CONTROL_SEQ_ESCAPE_SUFFIX,  // 21
        CONTROL_SEQ_ESCAPE_PREFIX, '1', '6', CONTROL_SEQ_ESCAPE_SUFFIX,  // 22
        CONTROL_SEQ_ESCAPE_PREFIX, '1', '7', CONTROL_SEQ_ESCAPE_SUFFIX,  // 23
        CONTROL_SEQ_ESCAPE_PREFIX, '1', '8', CONTROL_SEQ_ESCAPE_SUFFIX,  // 24
        CONTROL_SEQ_ESCAPE_PREFIX, '1', '9', CONTROL_SEQ_ESCAPE_SUFFIX,  // 25
        CONTROL_SEQ_ESCAPE_PREFIX, '1', 'a', CONTROL_SEQ_ESCAPE_SUFFIX,  // 26
        CONTROL_SEQ_ESCAPE_PREFIX, '1', 'b', CONTROL_SEQ_ESCAPE_SUFFIX,  // 27
        CONTROL_SEQ_ESCAPE_PREFIX, '1', 'c', CONTROL_SEQ_ESCAPE_SUFFIX,  // 28
        CONTROL_SEQ_ESCAPE_PREFIX, '1', 'd', CONTROL_SEQ_ESCAPE_SUFFIX,  // 29
        CONTROL_SEQ_ESCAPE_PREFIX, '1', 'e', CONTROL_SEQ_ESCAPE_SUFFIX,  // 30
        CONTROL_SEQ_ESCAPE_PREFIX, '1', 'f', CONTROL_SEQ_ESCAPE_SUFFIX,  // 31
        CONTROL_SEQ_ESCAPE_FULL_ZERO_REPEAT2,                            // 32 ~ 33
        '\\', '"', CONTROL_SEQ_ESCAPE_MIDDLE, CONTROL_SEQ_ESCAPE_SUFFIX, // 34
        CONTROL_SEQ_ESCAPE_FULL_ZERO_REPEAT32,                           // 35 ~ 66
        CONTROL_SEQ_ESCAPE_FULL_ZERO_REPEAT16,                           // 67 ~ 82
        CONTROL_SEQ_ESCAPE_FULL_ZERO_REPEAT8,                            // 83 ~ 90
        CONTROL_SEQ_ESCAPE_FULL_ZERO,                                    // 91
        '\\', '\\'                                                       // 92
};
#endif // COMPILE_READ_UCS_LEVEL == 1 && COMPILE_INDENT_LEVEL == 0


// define: SIMD_EXTRACT_PART, SIMD_MASK_EXTRACT_PART
#if SIMD_BIT_SIZE == 512
#define SIMD_EXTRACT_PART _mm512_extracti32x4_epi32
#define SIMD_MASK_EXTRACT_PART(m, i) (u16)((m >> (i * 16)) & 0xFFFF)
#elif SIMD_BIT_SIZE == 256
#define SIMD_EXTRACT_PART _mm256_extracti128_si256
#define SIMD_MASK_EXTRACT_PART SIMD_EXTRACT_PART
#else
#define SIMD_EXTRACT_PART assert(false)
#define SIMD_MASK_EXTRACT_PART assert(false)
#endif


force_noinline UnicodeVector *VECTOR_WRITE_ESCAPE_IMPL(StackVars *stack_vars, const _FROM_TYPE *src, Py_ssize_t len, Py_ssize_t additional_len) {
    UnicodeVector *vec = GET_VEC(stack_vars);
    _TARGET_TYPE *writer = _WRITER(vec);
    const _FROM_TYPE *src_end = src + len;
    while (src < src_end) {
        _TARGET_TYPE srcval = (_TARGET_TYPE) *src;
        usize unicode_point = (usize) srcval;
        Py_ssize_t copy_count = (unicode_point <= _Slash) ? _ControlJump[unicode_point] : 0;
        if (likely(!copy_count)) {
            *writer++ = srcval;
        } else {
            _TARGET_TYPE *copy_ptr = &_CONTROL_SEQ_TABLE[unicode_point * 8];
            if (copy_count == 2) {
                _WRITER(vec) = writer;
                vec = VEC_RESERVE(stack_vars, 2 + len + TAIL_PADDING / sizeof(_TARGET_TYPE) + additional_len);
                RETURN_ON_UNLIKELY_ERR(!vec);
                writer = _WRITER(vec);
                memcpy((void *) writer, (const void *) copy_ptr, 2 * sizeof(_TARGET_TYPE));
                writer += 2;
            } else {
                assert(6 == copy_count);
                _WRITER(vec) = writer;
                vec = VEC_RESERVE(stack_vars, 6 + len + TAIL_PADDING / sizeof(_TARGET_TYPE) + additional_len);
                RETURN_ON_UNLIKELY_ERR(!vec);
                writer = _WRITER(vec);
#if COMPILE_WRITE_UCS_LEVEL == 1 || SIZEOF_VOID_P == 8
                memcpy((void *) writer, (const void *) copy_ptr, 8 * sizeof(_TARGET_TYPE));
#else //  COMPILE_WRITE_UCS_LEVEL == 1 || SIZEOF_VOID_P == 8
                memcpy((void *) writer, (const void *) copy_ptr, 6 * sizeof(_TARGET_TYPE));
#endif
                writer += 6;
            }
        }
        src++;
    }
    _WRITER(vec) = writer;
    return vec;
}

#if COMPILE_READ_UCS_LEVEL != COMPILE_WRITE_UCS_LEVEL && COMPILE_INDENT_LEVEL == 0
force_inline void WRITE_SIMD_WITH_TAIL_LEN(_TARGET_TYPE *dst, SIMD_TYPE SIMD_VAR, Py_ssize_t len) {
#if COMPILE_READ_UCS_LEVEL == COMPILE_WRITE_UCS_LEVEL
    static_assert(false, "Unreachable for compiler");
#else
#if COMPILE_READ_UCS_LEVEL == 2
// 2->4
#if SIMD_BIT_SIZE == 512
    assert(false);
    // // 256->512
    // __m256i _y;
    // __m512i _z;
    // // 0
    // _y = SIMD_EXTRACT_HALF(SIMD_VAR, 0);
    // _z = elevate_2_4_to_512(_y);
    // write_512(dst, _z);
    // dst += CHECK_COUNT_MAX / 2;
    // // 1
    // _y = SIMD_EXTRACT_HALF(SIMD_VAR, 1);
    // _z = elevate_2_4_to_512(_y);
    // write_512(dst, _z);
    // dst += CHECK_COUNT_MAX / 2;
#elif SIMD_BIT_SIZE == 256
    // 2 * CHECK_COUNT_MAX
    // 128->256
    __m128i _x;
    __m256i _y;
    __m256i writemask;
    Py_ssize_t partlen;
    // 0
    partlen = len - CHECK_COUNT_MAX / 2;
    partlen = partlen > 0 ? (2 * partlen) : 0;
    _x = SIMD_EXTRACT_HALF(SIMD_VAR, 0);
    _y = elevate_2_4_to_256(_x);
    writemask = load_256_aligned(&_MaskTable_32[(usize) (CHECK_COUNT_MAX - partlen)][0]);
    write_simd_256_with_writemask_4(dst, _y, writemask);
    // write_simd((void *) dst, _y);
    dst += CHECK_COUNT_MAX / 2;
    // 1
    partlen = (len < CHECK_COUNT_MAX / 2) ? (2 * len) : CHECK_COUNT_MAX;
    _x = SIMD_EXTRACT_HALF(SIMD_VAR, 1);
    _y = elevate_2_4_to_256(_x);
    writemask = load_256_aligned(&_MaskTable_32[(usize) (CHECK_COUNT_MAX - partlen)][0]);
    write_simd_256_with_writemask_4(dst, _y, writemask);
    // write_simd((void *) dst, _y);
    dst += CHECK_COUNT_MAX / 2;
#else  // SIMD_BIT_SIZE == 128
    assert(false);
    // 64(128)->128
    // __m128i _x;
    // // 0
    // _x = elevate_2_4_to_128(SIMD_VAR);
// write_simd((void *) dst, _x);
// dst += CHECK_COUNT_MAX / 2;
// // 1
// RIGHT_SHIFT_128BITS(SIMD_VAR, 64, &SIMD_VAR);
// _x = elevate_2_4_to_128(SIMD_VAR);
// write_simd((void *) dst, _x);
// dst += CHECK_COUNT_MAX / 2;
#endif // SIMD_BIT_SIZE
#else  // COMPILE_READ_UCS_LEVEL == 1
#if COMPILE_WRITE_UCS_LEVEL == 2
// 1->2
#if SIMD_BIT_SIZE == 512
    assert(false);
    // 256->512
    __m512i _z;
    __m256i _y;
    // 0
    _y = SIMD_EXTRACT_HALF(SIMD_VAR, 0);
    _z = elevate_1_2_to_512(_y);
    write_512(dst, _z);
    dst += CHECK_COUNT_MAX / 2;
    // 1
    _y = SIMD_EXTRACT_HALF(SIMD_VAR, 1);
    _z = elevate_1_2_to_512(_y);
    write_512(dst, _z);
    dst += CHECK_COUNT_MAX / 2;
#elif SIMD_BIT_SIZE == 256
    // 128->256
    __m128i _x;
    __m256i _y;
    __m256i writemask;
    Py_ssize_t partlen;
    // 0
    partlen = len - CHECK_COUNT_MAX / 2;
    partlen = partlen > 0 ? (partlen) : 0;
    _x = SIMD_EXTRACT_HALF(SIMD_VAR, 0);
    _y = elevate_1_2_to_256(_x);
    writemask = load_256_aligned(&_MaskTable_16[(usize) (CHECK_COUNT_MAX / 2 - partlen)][0]);
    write_simd_256_with_writemask_2(dst, _y, writemask);
    // write_simd((void *) dst, _y);
    dst += CHECK_COUNT_MAX / 2;
    // 1
    partlen = (len < CHECK_COUNT_MAX / 2) ? (len) : (CHECK_COUNT_MAX / 2);
    _x = SIMD_EXTRACT_HALF(SIMD_VAR, 1);
    _y = elevate_1_2_to_256(_x);
    writemask = load_256_aligned(&_MaskTable_16[(usize) (CHECK_COUNT_MAX / 2 - partlen)][0]);
    write_simd_256_with_writemask_2(dst, _y, writemask);
    // write_simd((void *) dst, _y);
    dst += CHECK_COUNT_MAX / 2;
#else  // SIMD_BIT_SIZE == 128
    // 64(128)->128
    __m128i _x;
    _x = elevate_1_2_to_128(SIMD_VAR);
    write_simd((void *) dst, _x);
    dst += CHECK_COUNT_MAX / 2;
    RIGHT_SHIFT_128BITS(SIMD_VAR, 64, &SIMD_VAR);
    _x = elevate_1_2_to_128(SIMD_VAR);
    write_simd((void *) dst, _x);
    dst += CHECK_COUNT_MAX / 2;
#endif // SIMD_BIT_SIZE
#else  // COMPILE_WRITE_UCS_LEVEL == 4
// 1->4
#if SIMD_BIT_SIZE == 512
    assert(false);
    // 128->512
    __m512i _z;
    __m128i _x;
    // 0
    _x = SIMD_EXTRACT_PART(SIMD_VAR, 0);
    _z = elevate_1_4_to_512(_x);
    write_512(dst, _z);
    dst += CHECK_COUNT_MAX / 4;
    // 1
    _x = SIMD_EXTRACT_PART(SIMD_VAR, 1);
    _z = elevate_1_4_to_512(_x);
    write_512(dst, _z);
    dst += CHECK_COUNT_MAX / 4;
    // 2
    _x = SIMD_EXTRACT_PART(SIMD_VAR, 2);
    _z = elevate_1_4_to_512(_x);
    write_512(dst, _z);
    dst += CHECK_COUNT_MAX / 4;
    // 3
    _x = SIMD_EXTRACT_PART(SIMD_VAR, 3);
    _z = elevate_1_4_to_512(_x);
    write_512(dst, _z);
    dst += CHECK_COUNT_MAX / 4;
#elif SIMD_BIT_SIZE == 256
    // 64(128)->256
    __m128i _x;
    __m256i _y;
    __m256i writemask;
    Py_ssize_t partlen;
    // 0
    partlen = len - CHECK_COUNT_MAX * 3 / 4;
    partlen = partlen > 0 ? (4 * partlen) : 0;
    _x = SIMD_EXTRACT_HALF(SIMD_VAR, 0);
    _y = elevate_1_4_to_256(_x);
    writemask = load_256_aligned(&_MaskTable_8[(usize) (CHECK_COUNT_MAX - partlen)][0]);
    write_simd_256_with_writemask_4(dst, _y, writemask);
    // write_simd((void *) dst, _y);
    dst += CHECK_COUNT_MAX / 2;
    // 1
    partlen = len - CHECK_COUNT_MAX / 2;
    partlen = partlen > 0 ? partlen : 0;
    partlen = partlen < CHECK_COUNT_MAX / 4 ? (4 * partlen) : CHECK_COUNT_MAX;
    RIGHT_SHIFT_128BITS(_x, 64, &_x);
    _y = elevate_1_4_to_256(_x);
    writemask = load_256_aligned(&_MaskTable_8[(usize) (CHECK_COUNT_MAX - partlen)][0]);
    write_simd_256_with_writemask_4(dst, _y, writemask);
    // write_simd((void *) dst, _y);
    dst += CHECK_COUNT_MAX / 2;
    // 2
    partlen = len - CHECK_COUNT_MAX / 4;
    partlen = partlen > 0 ? partlen : 0;
    partlen = partlen < CHECK_COUNT_MAX / 4 ? (4 * partlen) : CHECK_COUNT_MAX;
    _x = SIMD_EXTRACT_HALF(SIMD_VAR, 1);
    _y = elevate_1_4_to_256(_x);
    writemask = load_256_aligned(&_MaskTable_8[(usize) (CHECK_COUNT_MAX - partlen)][0]);
    write_simd_256_with_writemask_4(dst, _y, writemask);
    // write_simd((void *) dst, _y);
    dst += CHECK_COUNT_MAX / 2;
    // 3
    partlen = len;
    partlen = partlen < CHECK_COUNT_MAX / 4 ? (4 * partlen) : CHECK_COUNT_MAX;
    RIGHT_SHIFT_128BITS(_x, 64, &_x);
    _y = elevate_1_4_to_256(_x);
    writemask = load_256_aligned(&_MaskTable_8[(usize) (CHECK_COUNT_MAX - partlen)][0]);
    write_simd_256_with_writemask_4(dst, _y, writemask);
    // write_simd((void *) dst, _y);
    dst += CHECK_COUNT_MAX / 2;
#else  // SIMD_BIT_SIZE == 128
    // 32(128)->128
    // 0
    __m128i _x;
    _x = elevate_1_2_to_128(SIMD_VAR);
    write_simd((void *) dst, _x);
    dst += CHECK_COUNT_MAX / 2;
    // 1
    RIGHT_SHIFT_128BITS(SIMD_VAR, 32, &SIMD_VAR);
    _x = elevate_1_2_to_128(SIMD_VAR);
    write_simd((void *) dst, _x);
    dst += CHECK_COUNT_MAX / 2;
    // 2
    RIGHT_SHIFT_128BITS(SIMD_VAR, 32, &SIMD_VAR);
    _x = elevate_1_2_to_128(SIMD_VAR);
    write_simd((void *) dst, _x);
    dst += CHECK_COUNT_MAX / 2;
    // 3
    RIGHT_SHIFT_128BITS(SIMD_VAR, 32, &SIMD_VAR);
    _x = elevate_1_2_to_128(SIMD_VAR);
    write_simd((void *) dst, _x);
    dst += CHECK_COUNT_MAX / 2;
#endif // SIMD_BIT_SIZE
#endif

#endif
#endif
}
#endif // COMPILE_READ_UCS_LEVEL != COMPILE_WRITE_UCS_LEVEL && COMPILE_INDENT_LEVEL == 0


force_inline UnicodeVector *VECTOR_WRITE_UNICODE_TRAILING_IMPL(const _FROM_TYPE *src, Py_ssize_t len, StackVars *stack_vars) {
    UnicodeVector *vec = GET_VEC(stack_vars);
#if SIMD_BIT_SIZE == 512
    SIMD_512 z;
#if COMPILE_READ_UCS_LEVEL == 1
#define _MASKZ_LOADU _mm512_maskz_loadu_epi8
#define _MASK_STOREU _mm512_mask_storeu_epi8
#elif COMPILE_READ_UCS_LEVEL == 2
#define _MASKZ_LOADU _mm512_maskz_loadu_epi16
#define _MASK_STOREU _mm512_mask_storeu_epi16
#else // COMPILE_READ_UCS_LEVEL == 4
#define _MASKZ_LOADU _mm512_maskz_loadu_epi16
#define _MASK_STOREU _mm512_mask_storeu_epi16
#endif
    CHECK_MASK_TYPE rw_mask, tail_mask;
    rw_mask = ((CHECK_MASK_TYPE) 1 << (usize) len) - 1;
    z = _MASKZ_LOADU(rw_mask, (const void *) src);
    tail_mask = CHECK_ESCAPE_TAIL_IMPL_GET_MASK_512(z, rw_mask);
    if (likely(CHECK_MASK_ZERO(tail_mask))) {
#if COMPILE_READ_UCS_LEVEL == COMPILE_WRITE_UCS_LEVEL
        _MASK_STOREU((void *) _WRITER(vec), rw_mask, z);
#else
        MASK_ELEVATE_WRITE_512(_WRITER(vec), z, len);
#endif
    } else {
#if COMPILE_READ_UCS_LEVEL == 1
        usize tzcnt = (usize) tzcnt_u64(tail_mask);
#else
        usize tzcnt = (usize) tzcnt_u32((u32) tail_mask);
#endif
        assert(tzcnt < len);
#if COMPILE_READ_UCS_LEVEL == COMPILE_WRITE_UCS_LEVEL
        _MASK_STOREU((void *) _WRITER(vec), ((CHECK_MASK_TYPE) 1 << tzcnt) - 1, z);
#else
        if(tzcnt) MASK_ELEVATE_WRITE_512(_WRITER(vec), z, tzcnt);
#endif
        vec = VECTOR_WRITE_ESCAPE_IMPL(stack_vars, src + tzcnt, len - tzcnt, 0);
        RETURN_ON_UNLIKELY_ERR(!vec);
    }
#undef _MASKZ_LOADU
#undef _MASK_STOREU
#elif SIMD_BIT_SIZE == 256
    __m256i y;
    const _FROM_TYPE *load_start = src + len - CHECK_COUNT_MAX;
    _TARGET_TYPE *store_start = _WRITER(vec) + len - CHECK_COUNT_MAX;
    __m256i mask, check_mask;
#if COMPILE_READ_UCS_LEVEL == 1
    mask = load_256_aligned((void *) &_MaskTable_8[(usize) (CHECK_COUNT_MAX - len)][0]);
#elif COMPILE_READ_UCS_LEVEL == 2
    mask = load_256_aligned((void *) &_MaskTable_16[(usize) (CHECK_COUNT_MAX - len)][0]);
#else
    mask = load_256_aligned((void *) &_MaskTable_32[(usize) (CHECK_COUNT_MAX - len)][0]);
#endif
    check_mask = CHECK_ESCAPE_IMPL_GET_MASK(load_start, &y);
    check_mask = simd_and_256(check_mask, mask);
    if (likely(CHECK_MASK_ZERO(check_mask))) {
#if COMPILE_READ_UCS_LEVEL == COMPILE_WRITE_UCS_LEVEL
        WRITE_SIMD_256_WITH_WRITEMASK(store_start, y, mask);
#else
        WRITE_SIMD_WITH_TAIL_LEN(store_start, y, len);
#endif
        _WRITER(vec) += len;
    } else {
        vec = VECTOR_WRITE_ESCAPE_IMPL(stack_vars, src, len, 0);
        RETURN_ON_UNLIKELY_ERR(!vec);
    }
#else // SIMD_BIT_SIZE == 128
    // TODO
    assert(len < CHECK_COUNT_MAX);
    SIMD_128 x, mask, check_mask;
    const _FROM_TYPE *load_start = src + len - CHECK_COUNT_MAX;
    _TARGET_TYPE *store_start = _WRITER(vec) + len - CHECK_COUNT_MAX;
#if COMPILE_READ_UCS_LEVEL == 1
    mask = load_128_aligned((const void *) &_MaskTable_8[(usize) (CHECK_COUNT_MAX - len)][0]);
#elif COMPILE_READ_UCS_LEVEL == 2
    mask = load_128_aligned((const void *) &_MaskTable_16[(usize) (CHECK_COUNT_MAX - len)][0]);
#else
    mask = load_128_aligned((const void *) &_MaskTable_32[(usize) (CHECK_COUNT_MAX - len)][0]);
#endif
    check_mask = CHECK_ESCAPE_IMPL_GET_MASK(load_start, &x);
    check_mask = simd_and_128(check_mask, mask);
    if (likely(CHECK_MASK_ZERO(check_mask))) {
#if COMPILE_READ_UCS_LEVEL == COMPILE_WRITE_UCS_LEVEL
#if __SSE4_1__
        x = blendv_128(load_128((const void *) store_start), x, mask);
        write_128((void *) store_start, x);
#else  // < __SSE4_1__
        x = runtime_right_shift_128bits(x, COMPILE_READ_UCS_LEVEL * (int) (CHECK_COUNT_MAX - len));
        write_128(_WRITER(vec), x);
#endif // __SSE4_1__
#else  // COMPILE_READ_UCS_LEVEL != COMPILE_WRITE_UCS_LEVEL
        x = runtime_right_shift_128bits(x, COMPILE_READ_UCS_LEVEL * (int) (CHECK_COUNT_MAX - len));
        WRITE_SIMD_IMPL(_WRITER(vec), x);
#endif // COMPILE_READ_UCS_LEVEL == COMPILE_WRITE_UCS_LEVEL
        _WRITER(vec) += len;
    } else {
        vec = VECTOR_WRITE_ESCAPE_IMPL(stack_vars, src, len, 0);
        RETURN_ON_UNLIKELY_ERR(!vec);
    }
#endif
    return vec;
}

static_assert(sizeof(PyASCIIObject) >= 16, "sizeof(PyASCIIObject) == ?");
#if SIZEOF_VOID_P == 8
// avx and above may be enabled.
static_assert(sizeof(PyASCIIObject) >= 32, "sizeof(PyASCIIObject) == ?");
#endif

force_inline UnicodeVector *VECTOR_WRITE_UNICODE_IMPL(StackVars *stack_vars, _FROM_TYPE *src, Py_ssize_t len) {
    UnicodeVector *vec = GET_VEC(stack_vars);
    usize total_size = (usize) len;
    __m128i x;
#if SIMD_BIT_SIZE >= 256
    __m256i y;
#endif
#if SIMD_BIT_SIZE >= 512
    __m512i z;
#endif
    SIMD_MASK_TYPE mask;
    SIMD_SMALL_MASK_TYPE small_mask;
    SIMD_BIT_MASK_TYPE bit_mask;
    bool _c;
    while (len >= CHECK_COUNT_MAX) {
        mask = CHECK_ESCAPE_IMPL_GET_MASK(src, &SIMD_VAR);
        WRITE_SIMD_IMPL(_WRITER(vec), SIMD_VAR);
        if (likely(CHECK_MASK_ZERO(mask))) {
            src += CHECK_COUNT_MAX;
            _WRITER(vec) += CHECK_COUNT_MAX;
            len -= CHECK_COUNT_MAX;
        } else {
#if SIMD_BIT_SIZE == 512
            bit_mask = mask;
            assert(bit_mask);
            u64 done_count = tzcnt_u64(bit_mask) / sizeof(_FROM_TYPE);
#elif SIMD_BIT_SIZE == 256
            // for bit size < 512, we don't have cmp_epu8, the mask is calculated by subs_epu8
            // so we have to cmpeq with zero to get the real bit mask.
            mask = cmpeq0_8_256(mask);
            bit_mask = to_bitmask_256(mask);
            bit_mask = ~bit_mask;
            assert(bit_mask);
            u32 done_count = tzcnt_u32(bit_mask) / sizeof(_FROM_TYPE);
#else // SIMD_BIT_SIZE
#if COMPILE_READ_UCS_LEVEL != 4
            // for bit size < 512, we don't have cmp_epu8,
            // the mask is calculated by subs_epu for ucs < 4
            // so we have to cmpeq with zero to get the real bit mask.
            mask = cmpeq0_8_128(mask);
            bit_mask = to_bitmask_128(mask);
            bit_mask = ~bit_mask;
#else
            // ucs4 does not have subs_epu, so we don't need cmpeq0.
            // The mask itself is ready for use
            bit_mask = to_bitmask_128(mask);
#endif // COMPILE_READ_UCS_LEVEL
            assert(bit_mask);
            u32 done_count = tzcnt_u32((u32) bit_mask) / sizeof(_FROM_TYPE);
            // vec = VECTOR_WRITE_ESCAPE_IMPL(stack_vars, src, CHECK_COUNT_MAX, len - CHECK_COUNT_MAX);
            // src += CHECK_COUNT_MAX;
#endif
            assert(len >= done_count);
            len -= done_count;
            src += done_count;
            _WRITER(vec) += done_count;
            Py_ssize_t process_escape_count = PYYJSON_ENCODE_ESCAPE_ONCE_BYTES / sizeof(_FROM_TYPE);
            process_escape_count = process_escape_count > len ? len : process_escape_count;
            vec = VECTOR_WRITE_ESCAPE_IMPL(stack_vars, src, process_escape_count, len - process_escape_count);
            RETURN_ON_UNLIKELY_ERR(!vec);
            len -= process_escape_count;
            src += process_escape_count;
        }
    }
    if (!len) goto done;
    vec = VECTOR_WRITE_UNICODE_TRAILING_IMPL(src, len, stack_vars);
    RETURN_ON_UNLIKELY_ERR(!vec);
done:;
    assert(vec_in_boundary(vec));
    return vec;
    // #undef SIMD_VAR
}

// avoid compile again
#if COMPILE_READ_UCS_LEVEL == 1
force_inline void VECTOR_WRITE_INDENT(StackVars *stack_vars) {
#if COMPILE_INDENT_LEVEL > 0
    UnicodeVector *vec = GET_VEC(stack_vars);
    _TARGET_TYPE *writer = _WRITER(vec);
    *writer++ = '\n';
    usize cur_nested_depth = (usize) stack_vars->cur_nested_depth;
    for (usize i = 0; i < cur_nested_depth; i++) {
        *writer++ = ' ';
        *writer++ = ' ';
#if COMPILE_INDENT_LEVEL == 4
        *writer++ = ' ';
        *writer++ = ' ';
#endif
    }
    _WRITER(vec) += COMPILE_INDENT_LEVEL * cur_nested_depth + 1;
#endif
}
#endif // COMPILE_READ_UCS_LEVEL == 1

force_inline bool PYYJSON_CONCAT4(vec_write_key, COMPILE_INDENT_LEVEL, COMPILE_READ_UCS_LEVEL, COMPILE_WRITE_UCS_LEVEL)(PyObject *key, Py_ssize_t len, StackVars *stack_vars) {
    static_assert(COMPILE_READ_UCS_LEVEL <= COMPILE_WRITE_UCS_LEVEL, "COMPILE_READ_UCS_LEVEL <= COMPILE_WRITE_UCS_LEVEL");
    UnicodeVector *vec = GET_VEC(stack_vars);
    assert(PyUnicode_GET_LENGTH(key) == len);
    vec = VEC_RESERVE(stack_vars, get_indent_char_count(stack_vars, COMPILE_INDENT_LEVEL) + 4 + len + TAIL_PADDING);
    RETURN_ON_UNLIKELY_ERR(!vec);
    VECTOR_WRITE_INDENT(stack_vars);
    *_WRITER(vec)++ = '"';
    vec = VECTOR_WRITE_UNICODE_IMPL(stack_vars, (_FROM_TYPE *) get_unicode_data(key), len);
    RETURN_ON_UNLIKELY_ERR(!vec);
    vec = VEC_RESERVE(stack_vars, 3 + TAIL_PADDING);
    RETURN_ON_UNLIKELY_ERR(!vec);
    _TARGET_TYPE *writer = _WRITER(vec);
    *writer++ = '"';
    *writer++ = ':';
#if COMPILE_INDENT_LEVEL > 0
    *writer++ = ' ';
#if SIZEOF_VOID_P == 8 || COMPILE_WRITE_UCS_LEVEL != 4
    *writer = 0;
#endif // SIZEOF_VOID_P == 8 || COMPILE_WRITE_UCS_LEVEL != 4
#endif // COMPILE_INDENT_LEVEL > 0
    _WRITER(vec) += (COMPILE_INDENT_LEVEL > 0) ? 3 : 2;
    assert(vec_in_boundary(vec));
    return true;
}

force_inline bool PYYJSON_CONCAT4(vec_write_str, COMPILE_INDENT_LEVEL, COMPILE_READ_UCS_LEVEL, COMPILE_WRITE_UCS_LEVEL)(PyObject *key, Py_ssize_t len, StackVars *stack_vars, bool is_in_obj) {
    static_assert(COMPILE_READ_UCS_LEVEL <= COMPILE_WRITE_UCS_LEVEL, "COMPILE_READ_UCS_LEVEL <= COMPILE_WRITE_UCS_LEVEL");
    UnicodeVector *vec = GET_VEC(stack_vars);
    assert(PyUnicode_GET_LENGTH(key) == len);
    if (is_in_obj) {
        vec = VEC_RESERVE(stack_vars, 3 + len + TAIL_PADDING);
        RETURN_ON_UNLIKELY_ERR(!vec);
    } else {
        vec = VEC_RESERVE(stack_vars, get_indent_char_count(stack_vars, COMPILE_INDENT_LEVEL) + 3 + len + TAIL_PADDING);
        RETURN_ON_UNLIKELY_ERR(!vec);
        VECTOR_WRITE_INDENT(stack_vars);
    }
    *_WRITER(vec)++ = '"';
    vec = VECTOR_WRITE_UNICODE_IMPL(stack_vars, (_FROM_TYPE *) get_unicode_data(key), len);
    RETURN_ON_UNLIKELY_ERR(!vec);
    vec = VEC_RESERVE(stack_vars, 2 + TAIL_PADDING);
    RETURN_ON_UNLIKELY_ERR(!vec);
    _TARGET_TYPE *writer = _WRITER(vec);
    *writer++ = '"';
    *writer++ = ',';
    _WRITER(vec) += 2;
    assert(vec_in_boundary(vec));
    return true;
}

#undef MASK_ELEVATE_WRITE_512
#undef WRITE_SIMD_IMPL
#undef WRITE_SIMD_WITH_TAIL_LEN
#undef WRITE_SIMD_256_WITH_WRITEMASK
#undef CHECK_MASK_ZERO_SMALL
#undef CHECK_MASK_ZERO
#undef CHECK_ESCAPE_IMPL_GET_MASK
#undef CHECK_COUNT_MAX
#undef _CONTROL_SEQ_TABLE
#undef CHECK_ESCAPE_TAIL_IMPL_GET_MASK_512
#undef SIMD_RIGHT_SHIFT
#undef MASK_RIGHT_SHIFT
#undef VECTOR_WRITE_INDENT
#undef VEC_RESERVE
#undef VECTOR_WRITE_ESCAPE_IMPL
#undef VECTOR_WRITE_UNICODE_TRAILING_IMPL
#undef VECTOR_WRITE_UNICODE_IMPL
#undef _FROM_TYPE
#undef CHECK_MASK_TYPE
#undef _TARGET_TYPE
#undef _WRITER
