#ifndef PYYJSON_ENCODE_SHARED_H
#define PYYJSON_ENCODE_SHARED_H

#include "encode.h"
#include "yyjson.h"
#include <stddef.h>

/*==============================================================================
 * Macros
 *============================================================================*/
#define static_inline static yyjson_inline
#define force_inline static yyjson_inline
// #define force_inline force_noinline
#define force_noinline yyjson_noinline
#define likely yyjson_likely
#define unlikely yyjson_unlikely

#define REPEAT_2(x) x, x,
#define REPEAT_4(x) REPEAT_2(x) REPEAT_2(x)
#define REPEAT_8(x) REPEAT_4(x) REPEAT_4(x)
#define REPEAT_16(x) REPEAT_8(x) REPEAT_8(x)
#define REPEAT_32(x) REPEAT_16(x) REPEAT_16(x)
#define REPEAT_64(x) REPEAT_32(x) REPEAT_32(x)

#define _Quote (34)
#define _Slash (92)
#define _MinusOne (-1)
#define ControlMax (32)


// #define COMMON_TYPE_DEF()                                          \
//     using usrc = UCSType_t<__from>;                                \
//     using udst = UCSType_t<__to>;                                  \
//      Py_ssize_t _SizeRatio = sizeof(udst) / sizeof(usrc); \
//     static_assert((sizeof(udst) % sizeof(usrc)) == 0)

// #define SIMD_COMMON_DEF(_BitSize)               \
//     COMMON_TYPE_DEF();                          \
//      Py_ssize_t _BitsMax = (_BitSize); \
//      Py_ssize_t _MoveSrc = _BitsMax / (sizeof(usrc) * 8)


// #define CVTEPU_COMMON_DEF(_BitSizeSrc, _BitSizeDst)                   \
//     COMMON_TYPE_DEF();                                                \
//      size_t _SrcReadBits = (_BitSizeSrc);                    \
//      size_t _DstWriteBits = (_BitSizeDst);                   \
//      Py_ssize_t _MoveSrc = _BitSizeSrc / (sizeof(usrc) * 8); \
//      Py_ssize_t _MoveDst = _BitSizeDst / (sizeof(udst) * 8)


/*==============================================================================
 * Types
 *============================================================================*/

/** Type define for primitive types. */
typedef float f32;
typedef double f64;
typedef int8_t i8;
typedef uint8_t u8;
typedef int16_t i16;
typedef uint16_t u16;
typedef int32_t i32;
typedef uint32_t u32;
typedef int64_t i64;
typedef uint64_t u64;
typedef size_t usize;

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

// template<UCSKind __uu>
// struct UCSType;

// template<>
// struct UCSType<UCSKind::UCS1> {
//     using type = u8;
//      static auto escape = UCSKind::UCS2;
// };

// template<>
// struct UCSType<UCSKind::UCS2> {
//     using type = u16;
//      static auto escape = UCSKind::UCS4;
// };

// template<>
// struct UCSType<UCSKind::UCS4> {
//     using type = u32;
//      static auto escape = UCSKind::UCS4_WithEscape;
// };

// template<>
// struct UCSType<UCSKind::UCS4_WithEscape> {
//     using type = u64;
// };


enum X86SIMDLevel {
    SSE2,
    SSE3,
    SSE4,
    AVX,
    AVX2,
    AVX512,
};


// template<UCSKind __type>
// using UCSType_t = typename UCSType<__type>::type;

// #define UCS_ESCAPE_KIND(__type) UCSType_t<UCSType<__type>::escape>

#define CONTROL_SEQ_ESCAPE_PREFIX _Slash, 'u', '0', '0'

// template<UCSKind __type>
// static u8 _ControlSeqTable[ControlMax * 6] = {
//         CONTROL_SEQ_ESCAPE_PREFIX, '0', '0', // 0
//         CONTROL_SEQ_ESCAPE_PREFIX, '0', '1', // 1
//         CONTROL_SEQ_ESCAPE_PREFIX, '0', '2', // 2
//         CONTROL_SEQ_ESCAPE_PREFIX, '0', '3', // 3
//         CONTROL_SEQ_ESCAPE_PREFIX, '0', '4', // 4
//         CONTROL_SEQ_ESCAPE_PREFIX, '0', '5', // 5
//         CONTROL_SEQ_ESCAPE_PREFIX, '0', '6', // 6
//         CONTROL_SEQ_ESCAPE_PREFIX, '0', '7', // 7
//         '\\', 'b', ' ', ' ', ' ', ' ',       // 8
//         '\\', 't', ' ', ' ', ' ', ' ',       // 9
//         '\\', 'n', ' ', ' ', ' ', ' ',       // 10
//         CONTROL_SEQ_ESCAPE_PREFIX, '0', 'b', // 11
//         '\\', 'f', ' ', ' ', ' ', ' ',       // 12
//         '\\', 'r', ' ', ' ', ' ', ' ',       // 13
//         CONTROL_SEQ_ESCAPE_PREFIX, '0', 'e', // 14
//         CONTROL_SEQ_ESCAPE_PREFIX, '0', 'f', // 15
//         CONTROL_SEQ_ESCAPE_PREFIX, '1', '0', // 16
//         CONTROL_SEQ_ESCAPE_PREFIX, '1', '1', // 17
//         CONTROL_SEQ_ESCAPE_PREFIX, '1', '2', // 18
//         CONTROL_SEQ_ESCAPE_PREFIX, '1', '3', // 19
//         CONTROL_SEQ_ESCAPE_PREFIX, '1', '4', // 20
//         CONTROL_SEQ_ESCAPE_PREFIX, '1', '5', // 21
//         CONTROL_SEQ_ESCAPE_PREFIX, '1', '6', // 22
//         CONTROL_SEQ_ESCAPE_PREFIX, '1', '7', // 23
//         CONTROL_SEQ_ESCAPE_PREFIX, '1', '8', // 24
//         CONTROL_SEQ_ESCAPE_PREFIX, '1', '9', // 25
//         CONTROL_SEQ_ESCAPE_PREFIX, '1', 'a', // 26
//         CONTROL_SEQ_ESCAPE_PREFIX, '1', 'b', // 27
//         CONTROL_SEQ_ESCAPE_PREFIX, '1', 'c', // 28
//         CONTROL_SEQ_ESCAPE_PREFIX, '1', 'd', // 29
//         CONTROL_SEQ_ESCAPE_PREFIX, '1', 'e', // 30
//         CONTROL_SEQ_ESCAPE_PREFIX, '1', 'f', // 31
// };

static Py_ssize_t _ControlJump[ControlMax] = {
        6, 6, 6, 6, 6, 6, 6, 6, 2, 2, 2, 6, 2, 2, 6, 6, // 0-15
        6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, // 16-31
};

static i32 _ControlLengthAdd[ControlMax] = {
        5, 5, 5, 5, 5, 5, 5, 5, 1, 1, 1, 5, 1, 1, 5, 5, // 0-15
        5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, // 16-31
};

// template<typename T, std::size_t n>
// auto _generate_mask_array() {
//     std::array<std::array<T, n>, n> res{};

//     for (int a = 0; a < n; ++a) {
//         for (int b = 0; b < n; ++b) {
//             res[a][b] = a > b ? static_cast<T>(-1) : static_cast<T>(0);
//         }
//     }

//     return res;
// }

// #define _MASK_ARRAY_SIZE (512 / 8 / sizeof(T))
// template<typename T>
// yyjson_align(64) inline std::array<std::array<T, _MASK_ARRAY_SIZE>, _MASK_ARRAY_SIZE> _MaskArray = _generate_mask_array<T, _MASK_ARRAY_SIZE>();
// #undef _MASK_ARRAY_SIZE

/*==============================================================================
 * Buffer
 *============================================================================*/

static_assert((PYYJSON_ENCODE_DST_BUFFER_INIT_SIZE % 64) == 0, "(PYYJSON_ENCODE_DST_BUFFER_INIT_SIZE % 64) == 0");

/* use 64 align for better simd r/w */
// yyjson_align(64) inline thread_local u64 _ThreadLocal_DstBuffer[PYYJSON_ENCODE_DST_BUFFER_INIT_SIZE / 8];

// typedef struct BufferInfo {
//     void *m_buffer_start;
//     Py_ssize_t m_size;

//     force_inline static BufferInfo init() {
//         BufferInfo ret{
//                 (void *) &_ThreadLocal_DstBuffer[0],
//                 PYYJSON_ENCODE_DST_BUFFER_INIT_SIZE,
//         };
//         return ret;
//     }

//     template<UCSKind __kind>
//     force_inline bool resizer(UCSType_t<__kind> *&dst, Py_ssize_t required_len) {
//         using uu = UCSType_t<__kind>;
//         assert(m_size >= 0);
//         assert(required_len > m_size);
//         assert((char *) dst >= (char *) this->m_buffer_start);
//         assert(((char *) dst - (char *) this->m_buffer_start) % static_cast<std::ptrdiff_t>(__kind) == 0);
//         std::ptrdiff_t dst_offset = this->calc_dst_offset<__kind>(dst);
//         Py_ssize_t _CurSizeMax = (PY_SSIZE_T_MAX & ~(Py_ssize_t) 3) / 3 * 2;
//         if (unlikely(this->m_size > _CurSizeMax || required_len > (PY_SSIZE_T_MAX / 4 * 4))) {
//             PyErr_NoMemory();
//             return false;
//         }
//         Py_ssize_t to_alloc = this->m_size + this->m_size / 2;
//         if (unlikely(to_alloc < required_len)) {
//             to_alloc = required_len;
//         }
//         to_alloc = (to_alloc + 3) & ~3;
//         // start alloc
//         void *new_buffer = malloc((size_t) to_alloc);
//         if (unlikely(!new_buffer)) {
//             PyErr_NoMemory();
//             return false;
//         }
//         size_t to_copy_len = (size_t) ((char *) dst - (char *) this->m_buffer_start);
//         memcpy(new_buffer, this->m_buffer_start, to_copy_len);
//         if (unlikely(!this->is_static_buffer())) {
//             free(this->m_buffer_start);
//         }
//         this->m_buffer_start = new_buffer;
//         this->m_size = to_alloc;
//         // move dst
//         dst = ((uu *) new_buffer) + dst_offset;
//         return true;
//     }

//     template<UCSKind __kind>
//     std::ptrdiff_t calc_dst_offset(UCSType_t<__kind> *dst) const {
//         using uu = UCSType_t<__kind>;
//         std::ptrdiff_t dst_offset = dst - (uu *) this->m_buffer_start;
//         return dst_offset;
//     }

//     template<UCSKind __kind>
//     std::ptrdiff_t calc_dst_offset_u8(UCSType_t<__kind> *dst) const {
//         std::ptrdiff_t dst_offset = (char *) dst - (char *) this->m_buffer_start;
//         return dst_offset;
//     }

//     force_inline bool is_static_buffer() {
//         return this->m_buffer_start == (void *) &_ThreadLocal_DstBuffer[0];
//     }

//     template<UCSKind __kind>
//     force_inline Py_ssize_t get_required_len_u8(UCSType_t<__kind> *dst, Py_ssize_t additional_ucs_count) {
//         return this->calc_dst_offset_u8<__kind>(dst) + additional_ucs_count * sizeof(UCSType_t<__kind>);
//     }

//     force_inline void release() {
//         if (unlikely(!this->is_static_buffer())) {
//             free(this->m_buffer_start);
//         }
//         this->m_buffer_start = nullptr;
//     }
// } BufferInfo;


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


/*==============================================================================
 * Constants
 *============================================================================*/

yyjson_align(64) const i8 _Quote_i8[64] = {REPEAT_64(_Quote)};
yyjson_align(64) const i16 _Quote_i16[32] = {REPEAT_32(_Quote)};
yyjson_align(64) const i32 _Quote_i32[16] = {REPEAT_16(_Quote)};
yyjson_align(64) const i8 _Slash_i8[64] = {REPEAT_64(_Slash)};
yyjson_align(64) const i16 _Slash_i16[32] = {REPEAT_32(_Slash)};
yyjson_align(64) const i32 _Slash_i32[16] = {REPEAT_16(_Slash)};
yyjson_align(64) const i8 _MinusOne_i8[64] = {REPEAT_64(_MinusOne)};
yyjson_align(64) const i16 _MinusOne_i16[32] = {REPEAT_32(_MinusOne)};
yyjson_align(64) const i32 _MinusOne_i32[16] = {REPEAT_16(_MinusOne)};
yyjson_align(64) const i8 _ControlMax_i8[64] = {REPEAT_64(ControlMax)};
yyjson_align(64) const i16 _ControlMax_i16[32] = {REPEAT_32(ControlMax)};
yyjson_align(64) const i32 _ControlMax_i32[16] = {REPEAT_16(ControlMax)};
yyjson_align(64) const i8 _Seven_i8[64] = {REPEAT_64(7)};
yyjson_align(64) const i16 _Seven_i16[32] = {REPEAT_32(7)};
yyjson_align(64) const i32 _Seven_i32[16] = {REPEAT_16(7)};
yyjson_align(64) const i8 _Eleven_i8[64] = {REPEAT_64(11)};
yyjson_align(64) const i16 _Eleven_i16[32] = {REPEAT_32(11)};
yyjson_align(64) const i32 _Eleven_i32[16] = {REPEAT_16(11)};
yyjson_align(64) const i8 _Fourteen_i8[64] = {REPEAT_64(14)};
yyjson_align(64) const i16 _Fourteen_i16[32] = {REPEAT_32(14)};
yyjson_align(64) const i32 _Fourteen_i32[16] = {REPEAT_16(14)};
yyjson_align(64) const i32 _All_0XFF[16] = {REPEAT_16(-1)};

yyjson_align(64) const u8 _MaskTable_8[64][64] = {
        {(u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1}};
yyjson_align(64) const u16 _MaskTable_16[32][32] = {
        {(u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1},
        {(u16) 0, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1},
        {(u16) 0, (u16) 0, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1},
        {(u16) 0, (u16) 0, (u16) 0, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1},
        {(u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1},
        {(u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1},
        {(u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1},
        {(u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1},
        {(u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1},
        {(u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1},
        {(u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1},
        {(u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1},
        {(u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1},
        {(u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1},
        {(u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1},
        {(u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1},
        {(u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1},
        {(u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1},
        {(u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1},
        {(u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1},
        {(u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1},
        {(u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1},
        {(u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1},
        {(u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1},
        {(u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1},
        {(u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1},
        {(u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1},
        {(u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) -1, (u16) -1, (u16) -1, (u16) -1, (u16) -1},
        {(u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) -1, (u16) -1, (u16) -1, (u16) -1},
        {(u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) -1, (u16) -1, (u16) -1},
        {(u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) -1, (u16) -1},
        {(u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) 0, (u16) -1}};
yyjson_align(64) const u32 _MaskTable_32[16][16] = {
        {(u32) -1, (u32) -1, (u32) -1, (u32) -1, (u32) -1, (u32) -1, (u32) -1, (u32) -1, (u32) -1, (u32) -1, (u32) -1, (u32) -1, (u32) -1, (u32) -1, (u32) -1, (u32) -1},
        {(u32) 0, (u32) -1, (u32) -1, (u32) -1, (u32) -1, (u32) -1, (u32) -1, (u32) -1, (u32) -1, (u32) -1, (u32) -1, (u32) -1, (u32) -1, (u32) -1, (u32) -1, (u32) -1},
        {(u32) 0, (u32) 0, (u32) -1, (u32) -1, (u32) -1, (u32) -1, (u32) -1, (u32) -1, (u32) -1, (u32) -1, (u32) -1, (u32) -1, (u32) -1, (u32) -1, (u32) -1, (u32) -1},
        {(u32) 0, (u32) 0, (u32) 0, (u32) -1, (u32) -1, (u32) -1, (u32) -1, (u32) -1, (u32) -1, (u32) -1, (u32) -1, (u32) -1, (u32) -1, (u32) -1, (u32) -1, (u32) -1},
        {(u32) 0, (u32) 0, (u32) 0, (u32) 0, (u32) -1, (u32) -1, (u32) -1, (u32) -1, (u32) -1, (u32) -1, (u32) -1, (u32) -1, (u32) -1, (u32) -1, (u32) -1, (u32) -1},
        {(u32) 0, (u32) 0, (u32) 0, (u32) 0, (u32) 0, (u32) -1, (u32) -1, (u32) -1, (u32) -1, (u32) -1, (u32) -1, (u32) -1, (u32) -1, (u32) -1, (u32) -1, (u32) -1},
        {(u32) 0, (u32) 0, (u32) 0, (u32) 0, (u32) 0, (u32) 0, (u32) -1, (u32) -1, (u32) -1, (u32) -1, (u32) -1, (u32) -1, (u32) -1, (u32) -1, (u32) -1, (u32) -1},
        {(u32) 0, (u32) 0, (u32) 0, (u32) 0, (u32) 0, (u32) 0, (u32) 0, (u32) -1, (u32) -1, (u32) -1, (u32) -1, (u32) -1, (u32) -1, (u32) -1, (u32) -1, (u32) -1},
        {(u32) 0, (u32) 0, (u32) 0, (u32) 0, (u32) 0, (u32) 0, (u32) 0, (u32) 0, (u32) -1, (u32) -1, (u32) -1, (u32) -1, (u32) -1, (u32) -1, (u32) -1, (u32) -1},
        {(u32) 0, (u32) 0, (u32) 0, (u32) 0, (u32) 0, (u32) 0, (u32) 0, (u32) 0, (u32) 0, (u32) -1, (u32) -1, (u32) -1, (u32) -1, (u32) -1, (u32) -1, (u32) -1},
        {(u32) 0, (u32) 0, (u32) 0, (u32) 0, (u32) 0, (u32) 0, (u32) 0, (u32) 0, (u32) 0, (u32) 0, (u32) -1, (u32) -1, (u32) -1, (u32) -1, (u32) -1, (u32) -1},
        {(u32) 0, (u32) 0, (u32) 0, (u32) 0, (u32) 0, (u32) 0, (u32) 0, (u32) 0, (u32) 0, (u32) 0, (u32) 0, (u32) -1, (u32) -1, (u32) -1, (u32) -1, (u32) -1},
        {(u32) 0, (u32) 0, (u32) 0, (u32) 0, (u32) 0, (u32) 0, (u32) 0, (u32) 0, (u32) 0, (u32) 0, (u32) 0, (u32) 0, (u32) -1, (u32) -1, (u32) -1, (u32) -1},
        {(u32) 0, (u32) 0, (u32) 0, (u32) 0, (u32) 0, (u32) 0, (u32) 0, (u32) 0, (u32) 0, (u32) 0, (u32) 0, (u32) 0, (u32) 0, (u32) -1, (u32) -1, (u32) -1},
        {(u32) 0, (u32) 0, (u32) 0, (u32) 0, (u32) 0, (u32) 0, (u32) 0, (u32) 0, (u32) 0, (u32) 0, (u32) 0, (u32) 0, (u32) 0, (u32) 0, (u32) -1, (u32) -1},
        {(u32) 0, (u32) 0, (u32) 0, (u32) 0, (u32) 0, (u32) 0, (u32) 0, (u32) 0, (u32) 0, (u32) 0, (u32) 0, (u32) 0, (u32) 0, (u32) 0, (u32) 0, (u32) -1}};


#endif // PYYJSON_ENCODE_SHARED_H
