#ifdef PYYJSON_CLANGD_DUMMY
#    include "decode/decode.h"
#    include "simd/simd_impl.h"
#    ifndef COMPILE_UCS_LEVEL
#        define COMPILE_UCS_LEVEL 0
#    endif
#    ifndef COMPILE_SIMD_BITS
#        define COMPILE_SIMD_BITS 128
#    endif
#endif

#if COMPILE_UCS_LEVEL == 0
#    define COMPILE_READ_UCS_LEVEL 1
#else
#    define COMPILE_READ_UCS_LEVEL COMPILE_UCS_LEVEL
#endif
#include "compile_context/sr_in.inl.h"

force_inline bool __check_vector_max_char_internal(vector_a vec, ReadStrState *read_state, u32 lowerbound_minus1, int string_type) {
    bool ret = checkmax(vec, lowerbound_minus1);
    if (unlikely(!ret)) {
        update_max_char_type(read_state, string_type);
    }
    return ret;
}

force_inline void check_vector_max_char(
        vector_a vec,
        ReadStrState *restrict read_state,
        bool need_mask, /* known at compile time */
        Py_ssize_t index /* only used when need_mask */) {
#if COMPILE_UCS_LEVEL == PYYJSON_STRING_TYPE_ASCII
    return;
#endif
    if (read_state->max_char_type == PYYJSON_STRING_TYPE_UCS4) {
        assert(false); // logic error
    }
    if (need_mask) {
#define LOAD_HEAD_MASK PYYJSON_CONCAT2(read_head_mask_table, READ_BIT_SIZE)
        // need a mask
        const void *mask_addr = LOAD_HEAD_MASK(index);
        vec = vec & *(vector_a *)mask_addr;
        // vec = SIMD_AND(load_simd(mask_addr), vec);
#undef LOAD_HEAD_MASK
    }
    switch (read_state->max_char_type) {
        case PYYJSON_STRING_TYPE_ASCII: {
#if COMPILE_UCS_LEVEL == PYYJSON_STRING_TYPE_UCS4
            __check_vector_max_char_internal(vec, read_state, 0xffff, PYYJSON_STRING_TYPE_UCS4) &&
#endif
#if COMPILE_UCS_LEVEL >= PYYJSON_STRING_TYPE_UCS2
                    __check_vector_max_char_internal(vec, read_state, 0xff, PYYJSON_STRING_TYPE_UCS2) &&
#endif
                    __check_vector_max_char_internal(vec, read_state, 0x7f, PYYJSON_STRING_TYPE_LATIN1);
            break;
        }
#if COMPILE_UCS_LEVEL > PYYJSON_STRING_TYPE_LATIN1
        case PYYJSON_STRING_TYPE_LATIN1: {
#    if COMPILE_UCS_LEVEL == PYYJSON_STRING_TYPE_UCS4
            __check_vector_max_char_internal(vec, read_state, 0xffff, PYYJSON_STRING_TYPE_UCS4) &&
#    endif
                    __check_vector_max_char_internal(vec, read_state, 0xff, PYYJSON_STRING_TYPE_UCS2);
            break;
        }
#endif
#if COMPILE_UCS_LEVEL > PYYJSON_STRING_TYPE_UCS2
        case PYYJSON_STRING_TYPE_UCS2: {
            __check_vector_max_char_internal(vec, read_state, 0xffff, PYYJSON_STRING_TYPE_UCS4);
            break;
        }
#endif
        // below are unreachable
        default: {
            PYYJSON_UNREACHABLE();
        }
    }
}

#undef COMPILE_READ_UCS_LEVEL
#include "compile_context/sr_out.inl.h"
