#include "encode_float.inl"

#ifndef COMPILE_WRITE_UCS_LEVEL
#error "COMPILE_WRITE_UCS_LEVEL is not defined"
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

#define VEC_RESERVE PYYJSON_CONCAT2(vec_reserve, COMPILE_WRITE_UCS_LEVEL)
#define _ELEVATE_U8_COPY PYYJSON_CONCAT2(_elevate_u8_copy, COMPILE_WRITE_UCS_LEVEL)

force_inline UnicodeVector *VEC_RESERVE(StackVars *stack_vars, Py_ssize_t size) {
    UnicodeVector *vec = GET_VEC(stack_vars);
    _TARGET_TYPE *target_ptr = _WRITER(vec) + size;
    if (unlikely(target_ptr > (_TARGET_TYPE *) VEC_END(vec))) {
        Py_ssize_t u8_diff = VEC_MEM_U8_DIFF(vec, target_ptr);
        assert(u8_diff >= 0);
        // Py_ssize_t target_size = u8_diff;
        Py_ssize_t target_size = VEC_MEM_U8_DIFF(vec, VEC_END(vec));
        assert(target_size >= 0);
        Py_ssize_t inc_size = target_size / 5;
        if (unlikely(target_size > (PY_SSIZE_T_MAX - inc_size))) {
            PyErr_NoMemory();
            return NULL;
        }
        target_size = target_size + inc_size;
        target_size = (target_size > u8_diff) ? target_size : u8_diff;
        Py_ssize_t w_diff = VEC_MEM_U8_DIFF(vec, _WRITER(vec));
        void *new_ptr = PyObject_Realloc((void *) vec, (size_t) target_size);
        if (unlikely(!new_ptr)) {
            PyErr_NoMemory();
            return NULL;
        }
        vec = (UnicodeVector *) new_ptr;
        GET_VEC(stack_vars) = vec;
        vec_set_rwptr_and_size(vec, w_diff, target_size);
    }
    return vec;
}

force_inline void _ELEVATE_U8_COPY(StackVars *stack_vars, u8 *buffer, Py_ssize_t len) {
#if COMPILE_WRITE_UCS_LEVEL > 1
    assert(len >= 0);
    while (len > 0) {
        *(_WRITER(stack_vars->vec))++ = *buffer++;
        len--;
    } // TODO
#else
    assert(false);
#endif
}

force_inline void PYYJSON_CONCAT2(vec_write_u64, COMPILE_WRITE_UCS_LEVEL)(StackVars *stack_vars, u64 val, usize sign) {
    assert(sign <= 1);
#if COMPILE_WRITE_UCS_LEVEL == 1
    UnicodeVector *vec = GET_VEC(stack_vars);
    u8 *buffer = _WRITER(vec);
#else
    u8 _buffer[32];
    u8 *buffer = _buffer;
#endif
    if (sign) *buffer = '-';
    u8 *buffer_end = write_u64(val, buffer + sign);
#if COMPILE_WRITE_UCS_LEVEL == 1
    vec->head.write_u8 = buffer_end;
#else
    Py_ssize_t write_len = buffer_end - buffer;
    _ELEVATE_U8_COPY(stack_vars, buffer, write_len);
#endif
}

force_inline void PYYJSON_CONCAT2(vec_write_f64, COMPILE_WRITE_UCS_LEVEL)(StackVars *stack_vars, u64 val_u64_repr) {
#if COMPILE_WRITE_UCS_LEVEL == 1
    UnicodeVector *vec = GET_VEC(stack_vars);
    u8 *buffer = _WRITER(vec);
#else
    u8 _buffer[32];
    u8 *buffer = _buffer;
#endif
    u8 *buffer_end = write_f64_raw(buffer, val_u64_repr);
#if COMPILE_WRITE_UCS_LEVEL == 1
    vec->head.write_u8 = buffer_end;
#else
    Py_ssize_t write_len = buffer_end - buffer;
    _ELEVATE_U8_COPY(stack_vars, buffer, write_len);
#endif
}

#undef _ELEVATE_U8_COPY
#undef VEC_RESERVE
#undef _TARGET_TYPE
#undef _WRITER
