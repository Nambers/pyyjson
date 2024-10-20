#include <immintrin.h>
#include "simd_detect.h"

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
#define _FROM_TYPE u32
#elif COMPILE_READ_UCS_LEVEL == 2
#define _FROM_TYPE u16
#elif COMPILE_READ_UCS_LEVEL == 1
#define _FROM_TYPE u8
#else
#error "COMPILE_READ_UCS_LEVEL must be 1, 2 or 4"
#endif

#define VECTOR_WRITE_UNICODE_IMPL PYYJSON_CONCAT3(vector_write_unicode_impl, COMPILE_READ_UCS_LEVEL, COMPILE_WRITE_UCS_LEVEL)
#define VECTOR_WRITE_ESCAPE_IMPL PYYJSON_CONCAT3(vector_write_escape_impl, COMPILE_READ_UCS_LEVEL, COMPILE_WRITE_UCS_LEVEL)


#define VEC_RESERVE PYYJSON_CONCAT2(vec_reserve, COMPILE_WRITE_UCS_LEVEL)

#define VECTOR_WRITE_INDENT PYYJSON_CONCAT3(vector_write_indent, COMPILE_INDENT_LEVEL, COMPILE_WRITE_UCS_LEVEL)

#define _CONTROL_SEQ_TABLE PYYJSON_CONCAT2(_ControlSeqTable, COMPILE_WRITE_UCS_LEVEL)

#define CHECK_COUNT_MAX (SIMD_BIT_SIZE / 8 / sizeof(_FROM_TYPE))
#define WRITE_COUNT_MAX (SIMD_BIT_SIZE / 8 / sizeof(_TARGET_TYPE))

#if COMPILE_INDENT_LEVEL == 0
// avoid compile again
#if COMPILE_READ_UCS_LEVEL == 1
// avoid compile again

static _TARGET_TYPE _CONTROL_SEQ_TABLE[ControlMax * 6] = {
        CONTROL_SEQ_ESCAPE_PREFIX, '0', '0', // 0
        CONTROL_SEQ_ESCAPE_PREFIX, '0', '1', // 1
        CONTROL_SEQ_ESCAPE_PREFIX, '0', '2', // 2
        CONTROL_SEQ_ESCAPE_PREFIX, '0', '3', // 3
        CONTROL_SEQ_ESCAPE_PREFIX, '0', '4', // 4
        CONTROL_SEQ_ESCAPE_PREFIX, '0', '5', // 5
        CONTROL_SEQ_ESCAPE_PREFIX, '0', '6', // 6
        CONTROL_SEQ_ESCAPE_PREFIX, '0', '7', // 7
        '\\', 'b', ' ', ' ', ' ', ' ',       // 8
        '\\', 't', ' ', ' ', ' ', ' ',       // 9
        '\\', 'n', ' ', ' ', ' ', ' ',       // 10
        CONTROL_SEQ_ESCAPE_PREFIX, '0', 'b', // 11
        '\\', 'f', ' ', ' ', ' ', ' ',       // 12
        '\\', 'r', ' ', ' ', ' ', ' ',       // 13
        CONTROL_SEQ_ESCAPE_PREFIX, '0', 'e', // 14
        CONTROL_SEQ_ESCAPE_PREFIX, '0', 'f', // 15
        CONTROL_SEQ_ESCAPE_PREFIX, '1', '0', // 16
        CONTROL_SEQ_ESCAPE_PREFIX, '1', '1', // 17
        CONTROL_SEQ_ESCAPE_PREFIX, '1', '2', // 18
        CONTROL_SEQ_ESCAPE_PREFIX, '1', '3', // 19
        CONTROL_SEQ_ESCAPE_PREFIX, '1', '4', // 20
        CONTROL_SEQ_ESCAPE_PREFIX, '1', '5', // 21
        CONTROL_SEQ_ESCAPE_PREFIX, '1', '6', // 22
        CONTROL_SEQ_ESCAPE_PREFIX, '1', '7', // 23
        CONTROL_SEQ_ESCAPE_PREFIX, '1', '8', // 24
        CONTROL_SEQ_ESCAPE_PREFIX, '1', '9', // 25
        CONTROL_SEQ_ESCAPE_PREFIX, '1', 'a', // 26
        CONTROL_SEQ_ESCAPE_PREFIX, '1', 'b', // 27
        CONTROL_SEQ_ESCAPE_PREFIX, '1', 'c', // 28
        CONTROL_SEQ_ESCAPE_PREFIX, '1', 'd', // 29
        CONTROL_SEQ_ESCAPE_PREFIX, '1', 'e', // 30
        CONTROL_SEQ_ESCAPE_PREFIX, '1', 'f', // 31
};
#endif


force_inline UnicodeVector *VECTOR_WRITE_ESCAPE_IMPL(StackVars *stack_vars, _FROM_TYPE *src, Py_ssize_t len, Py_ssize_t additional_len) {
    UnicodeVector *vec = GET_VEC(stack_vars);
    while (len) {
        if (*src == _Slash) {
            vec = VEC_RESERVE(stack_vars, 2 + len + TAIL_PADDING + additional_len);
            RETURN_ON_UNLIKELY_ERR(!vec);
            *_WRITER(vec)++ = '\\';
            *_WRITER(vec)++ = '\\';
        } else if (*src == _Quote) {
            vec = VEC_RESERVE(stack_vars, 2 + len + TAIL_PADDING + additional_len);
            RETURN_ON_UNLIKELY_ERR(!vec);
            *_WRITER(vec)++ = '\\';
            *_WRITER(vec)++ = '"';
        } else if (*src < 32) {
            vec = VEC_RESERVE(stack_vars, 6 + len + TAIL_PADDING + additional_len);
            RETURN_ON_UNLIKELY_ERR(!vec);
            Py_ssize_t len = _ControlJump[(usize) *src];
            assert(len == 6 || len == 2);
            if (len == 2) {
                memcpy((void *) _WRITER(vec), (void *) &_CONTROL_SEQ_TABLE[(usize) (*src) * 6], 2 * sizeof(_TARGET_TYPE));
                _WRITER(vec) += 2;
            } else {
                memcpy((void *) _WRITER(vec), (void *) &_CONTROL_SEQ_TABLE[(usize) (*src) * 6], 6 * sizeof(_TARGET_TYPE));
                _WRITER(vec) += 6;
            }
        } else {
            *_WRITER(vec)++ = *src;
        }
        len--;
        src++;
    }
    return vec;
}

force_inline UnicodeVector *VECTOR_WRITE_UNICODE_IMPL(StackVars *stack_vars, _FROM_TYPE *src, Py_ssize_t len) {
    UnicodeVector *vec = GET_VEC(stack_vars);
#define SIMD_VAR x
    __m128i x;
#if SIMD_BIT_SIZE >= 256
#undef SIMD_VAR
#define SIMD_VAR y
    __m256i y;
#endif
#if SIMD_BIT_SIZE >= 512
#undef SIMD_VAR
#define SIMD_VAR z
    __m512i z;
#endif

    bool _c;
    while (len >= CHECK_COUNT_MAX) {
        _c = CHECK_ESCAPE_IMPL(src, &SIMD_VAR);
        WRITE_SIMD_IMPL(_WRITER(vec), SIMD_VAR);
        if (likely(_c)) {
            src += CHECK_COUNT_MAX;
            _WRITER(vec) += WRITE_COUNT_MAX;
        } else {
#if SIMD_BIT_SIZE > 128
            const usize _Loop = SIMD_BIT_SIZE / 128;
            assert(_Loop > 1);
            for (usize i = 0; i < _Loop; ++i) {
                _c = CHECK_ESCAPE_128_IMPL(src, &x);
                WRITE_SIMD_128_IMPL(_WRITER(vec), x);
                if (likely(_c)) {
                    _WRITER(vec) += WRITE_COUNT_MAX / _Loop;
                } else {
                    vec = VECTOR_WRITE_ESCAPE_IMPL(stack_vars, src, CHECK_COUNT_MAX / _Loop, len - CHECK_COUNT_MAX / _Loop);
                }
                src += CHECK_COUNT_MAX / _Loop;
            }
#else
            vec = VECTOR_WRITE_ESCAPE_IMPL(stack_vars, src, CHECK_COUNT_MAX, len - CHECK_COUNT_MAX);
            src += CHECK_COUNT_MAX;
#endif
        }
        len -= CHECK_COUNT_MAX;
    }
    if (!len) return vec;

    _FROM_TYPE x_buf[CHECK_COUNT_MAX];
    pyyjson_memcpy_big_first((void *) x_buf, (void *) src, len * sizeof(_FROM_TYPE));
    pyyjson_memset_small_fisrt((void *) (((char *) x_buf) + len * sizeof(_FROM_TYPE)), 32, (CHECK_COUNT_MAX - len) * sizeof(_FROM_TYPE));
    _c = CHECK_ESCAPE_IMPL(src, &SIMD_VAR);
    WRITE_SIMD_IMPL(_WRITER(vec), SIMD_VAR);
    if (likely(_c)) {
        _WRITER(vec) += len;
    } else {
    }
    assert(vec_in_boundary(vec));
    return vec;
#undef SIMD_VAR
}

#endif

#if COMPILE_READ_UCS_LEVEL == 1
// avoid compile again
force_inline void VECTOR_WRITE_INDENT(StackVars *stack_vars) {
#if COMPILE_INDENT_LEVEL > 0
    UnicodeVector *vec = GET_VEC(stack_vars);
    *_WRITER(vec)++ = '\n';
    for (Py_ssize_t i = 0; i < stack_vars->cur_nested_depth; i++) {
        *_WRITER(vec)++ = ' ';
        *_WRITER(vec)++ = ' ';
#if COMPILE_INDENT_LEVEL == 4
        *_WRITER(vec)++ = ' ';
        *_WRITER(vec)++ = ' ';
#endif
    }
#endif
}
#endif

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
    *_WRITER(vec)++ = '"';
    *_WRITER(vec)++ = ':';
#if COMPILE_INDENT_LEVEL > 0
    *_WRITER(vec)++ = ' ';
#endif
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
    *_WRITER(vec)++ = '"';
    *_WRITER(vec)++ = ',';
    assert(vec_in_boundary(vec));
    return true;
}

#undef WRITE_COUNT_MAX
#undef CHECK_COUNT_MAX
#undef _CONTROL_SEQ_TABLE
#undef VECTOR_WRITE_INDENT
#undef VEC_RESERVE
#undef VECTOR_WRITE_ESCAPE_IMPL
#undef VECTOR_WRITE_UNICODE_IMPL
#undef _FROM_TYPE
#undef _TARGET_TYPE
#undef _WRITER
