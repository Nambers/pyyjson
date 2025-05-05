#define XXH_INLINE_ALL
#include "decode.h"

#include "decode_str_common.h"
#include "pyyjson.h"
#include "simd/cvt.h"
#include "simd/mask_table.h"
#include "simd/memcmp.h"
#include "simd/memcpy.h"
#include "simd/simd_impl.h"
#include "tls.h"
#include "xxhash.h"
#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <threads.h>
//
#include "decode_utils_wrap.inl.c"

extern thread_local u8 pyyjson_string_buffer[PYYJSON_STRING_BUFFER_SIZE];

static_assert((PYYJSON_STRING_BUFFER_SIZE % 64) == 0, "(PYYJSON_STRING_BUFFER_SIZE % 64) == 0");

// force_inline PyObject *read_bytes(const u8 **ptr, u8 *write_buffer, bool is_key);
// force_inline PyObject *read_bytes_root_pretty(const u8 *dat, usize len);

force_inline bool decode_ctn_is_arr(DecodeCtnWithSize *ctn) {
    return ctn->raw < 0;
}

force_inline Py_ssize_t get_decode_ctn_len(DecodeCtnWithSize *ctn) {
    return ctn->raw & PY_SSIZE_T_MAX;
}

force_inline void set_decode_ctn(DecodeCtnWithSize *ctn, Py_ssize_t len, bool is_arr) {
    assert(len >= 0);
    ctn->raw = len | (is_arr ? PY_SSIZE_T_MIN : 0);
}

force_inline void incr_decode_ctn_size(DecodeCtnWithSize *ctn) {
    assert(ctn->raw != PY_SSIZE_T_MAX);
    ctn->raw++;
}

force_inline bool ctn_grow_check(DecodeCtnStackInfo *decode_ctn_info) {
    return ++decode_ctn_info->ctn < decode_ctn_info->ctn_end;
}

#if PY_MINOR_VERSION >= 13
// these are hidden in Python 3.13
#    if PY_MINOR_VERSION == 13
PyAPI_FUNC(Py_hash_t) _Py_HashBytes(const void *, Py_ssize_t);
#    endif // PY_MINOR_VERSION == 13
PyAPI_FUNC(int) _PyDict_SetItem_KnownHash_LockHeld(PyObject *mp, PyObject *key, PyObject *item, Py_hash_t hash);
#    define _PyDict_SetItem_KnownHash _PyDict_SetItem_KnownHash_LockHeld
#endif // PY_MINOR_VERSION >= 13

#if PY_MINOR_VERSION >= 12
#    define PYYJSON_PY_DECREF_DEBUG() (_Py_DECREF_STAT_INC())
#    define PYYJSON_PY_INCREF_DEBUG() (_Py_INCREF_STAT_INC())
#else
#    ifdef Py_REF_DEBUG
#        define PYYJSON_PY_DECREF_DEBUG() (_Py_RefTotal--)
#        define PYYJSON_PY_INCREF_DEBUG() (_Py_RefTotal++)
#    else
#        define PYYJSON_PY_DECREF_DEBUG()
#        define PYYJSON_PY_INCREF_DEBUG()
#    endif
#endif
#define REHASHER(_x) (((size_t)(_x)) % (PYYJSON_KEY_CACHE_SIZE))

typedef XXH64_hash_t pyyjson_hash_t;


#if PYYJSON_ENABLE_TRACE
Py_ssize_t max_str_len = 0;
int __count_trace[PYYJSON_OP_BITCOUNT_MAX] = {0};
int __hash_trace[PYYJSON_KEY_CACHE_SIZE] = {0};
size_t __hash_hit_counter = 0;
size_t __hash_add_key_call_count = 0;

#    define PYYJSON_TRACE_STR_LEN(_len) max_str_len = max_str_len > _len ? max_str_len : _len
#    define PYYJSON_TRACE_HASH(_hash) \
        __hash_add_key_call_count++;  \
        __hash_trace[_hash & (PYYJSON_KEY_CACHE_SIZE - 1)]++
#    define PYYJSON_TRACE_CACHE_HIT() __hash_hit_counter++
#    define PYYJSON_TRACE_HASH_CONFLICT(_hash) printf("hash conflict: %lld, index=%lld\n", (long long int)_hash, (long long int)(_hash & (PYYJSON_KEY_CACHE_SIZE - 1)))
#else // PYYJSON_ENABLE_TRACE
#    define PYYJSON_TRACE_STR_LEN(_len) (void)(0)
#    define PYYJSON_TRACE_HASH(_hash) (void)(0)
#    define PYYJSON_TRACE_CACHE_HIT() (void)(0)
#    define PYYJSON_TRACE_HASH_CONFLICT(_hash) (void)(0)
#endif // PYYJSON_ENABLE_TRACE

force_inline void Py_DecRef_NoCheck(PyObject *op) {
    // Non-limited C API and limited C API for Python 3.9 and older access
    // directly PyObject.ob_refcnt.
#if PY_MINOR_VERSION >= 12
    if (_Py_IsImmortal(op)) {
        return;
    }
#endif
    PYYJSON_PY_DECREF_DEBUG();
    assert(op->ob_refcnt > 1);
    --op->ob_refcnt;
}

force_inline void Py_Immortal_IncRef(PyObject *op) {
    // Non-limited C API and limited C API for Python 3.9 and older access
    // directly PyObject.ob_refcnt.
#if PY_MINOR_VERSION >= 12
#    if SIZEOF_VOID_P > 4
    // Portable saturated add, branching on the carry flag and set low bits
#        if !defined(NDEBUG) && PY_MINOR_VERSION < 14
    assert(0 > (int32_t)op->ob_refcnt_split[PY_BIG_ENDIAN]);
#        endif // NDEBUG
#    else      // SIZEOF_VOID_P > 4
    // Explicitly check immortality against the immortal value
    assert(_Py_IsImmortal(op));
#    endif     // SIZEOF_VOID_P > 4
#else          // PY_MINOR_VERSION >= 12
    op->ob_refcnt++;
#endif         // PY_MINOR_VERSION >= 12
    PYYJSON_PY_INCREF_DEBUG();
}

extern pyyjson_cache_type AssociativeKeyCache[PYYJSON_KEY_CACHE_SIZE];

force_inline void add_key_cache(pyyjson_hash_t hash, PyObject *obj) {
    assert(PyUnicode_GET_LENGTH(obj) * PyUnicode_KIND(obj) <= 64);
    size_t index = REHASHER(hash);
    PYYJSON_TRACE_HASH(index);
    pyyjson_cache_type old = AssociativeKeyCache[index];
    if (old) {
        PYYJSON_TRACE_HASH_CONFLICT(hash);
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
        PYYJSON_TRACE_CACHE_HIT();
        return cache;
    }
    return NULL;
}

force_inline void make_hash(PyASCIIObject *ascii, const u8 *unicode_str, size_t real_len) {
#if PY_MINOR_VERSION >= 14
    ascii->hash = Py_HashBuffer(unicode_str, real_len);
#else
    ascii->hash = _Py_HashBytes(unicode_str, real_len);
#endif
}

force_inline PyObject *make_string(const u8 *unicode_str, Py_ssize_t len, int type_flag, bool is_key) {
    PYYJSON_TRACE_STR_LEN(len);
    PyObject *obj;
    XXH64_hash_t hash;
    size_t real_len;
    Py_ssize_t offset;
    Py_UCS4 max_char;
    int kind;
    bool ascii;

    switch (type_flag) {
        case PYYJSON_STRING_TYPE_ASCII: {
            ascii = true;
            kind = 1;
            max_char = 0x7f;
            real_len = len;
            offset = sizeof(PyASCIIObject);
            break;
        }
        case PYYJSON_STRING_TYPE_LATIN1: {
            ascii = false;
            kind = 1;
            max_char = 0xff;
            real_len = len;
            offset = sizeof(PyCompactUnicodeObject);
            break;
        }
        case PYYJSON_STRING_TYPE_UCS2: {
            ascii = false;
            kind = 2;
            max_char = 0xffff;
            real_len = len * 2;
            offset = sizeof(PyCompactUnicodeObject);
            break;
        }
        case PYYJSON_STRING_TYPE_UCS4: {
            ascii = false;
            kind = 4;
            max_char = 0x10ffff;
            real_len = len * 4;
            offset = sizeof(PyCompactUnicodeObject);
            break;
        }
        default:
            assert(false);
            Py_UNREACHABLE();
    }

    bool should_cache = (is_key && real_len && likely(real_len <= 64));

    if (should_cache) {
        hash = XXH3_64bits(unicode_str, real_len);
        obj = get_key_cache(unicode_str, hash, real_len, kind, ascii);
        if (obj) {
            Py_INCREF(obj);
            return obj;
        }
    }

    obj = PyUnicode_New(len, max_char);
    if (obj == NULL) return NULL;
    pyyjson_memcpy(PYYJSON_CAST(u8 *, obj) + offset, unicode_str, real_len);
    if (should_cache) {
        add_key_cache(hash, obj);
    }
success:
    if (is_key) {
        PyASCIIObject *ascii_obj = PYYJSON_CAST(PyASCIIObject *, obj);
        if (len) {
            assert(ascii_obj->hash == -1);
            make_hash(ascii_obj, unicode_str, real_len);
        } else {
            // empty unicode has zero hash
            assert(ascii_obj->hash != -1);
        }
    }
    return obj;
}

force_inline bool init_decode_obj_stack_info(DecodeObjStackInfo *restrict decode_obj_stack_info) {
    assert(!decode_obj_stack_info->result_stack);
    PyObject **new_buffer = get_decode_obj_stack_buffer();
    if (unlikely(!new_buffer)) {
        PyErr_NoMemory();
        return false;
    }
    decode_obj_stack_info->result_stack = new_buffer;
    decode_obj_stack_info->cur_write_result_addr = new_buffer;
    decode_obj_stack_info->result_stack_end = new_buffer + PYYJSON_DECODE_OBJ_BUFFER_INIT_SIZE;
    return true;
}

force_inline bool init_decode_ctn_stack_info(DecodeCtnStackInfo *restrict decode_ctn_stack_info) {
    assert(!decode_ctn_stack_info->ctn_start);
    DecodeCtnWithSize *new_buffer = get_decode_ctn_stack_buffer();
    if (unlikely(!new_buffer)) {
        PyErr_NoMemory();
        return false;
    }
    decode_ctn_stack_info->ctn_start = new_buffer;
    decode_ctn_stack_info->ctn = new_buffer;
    decode_ctn_stack_info->ctn_end = new_buffer + PYYJSON_DECODE_MAX_RECURSION;
    return true;
}

#if PYYJSON_ENABLE_TRACE
#    define PYYJSON_TRACE_OP(x)                                 \
        do {                                                    \
            for (int i = 0; i < PYYJSON_OP_BITCOUNT_MAX; i++) { \
                if (x & (1 << i)) {                             \
                    __count_trace[i]++;                         \
                    break;                                      \
                }                                               \
            }                                                   \
            __op_counter++;                                     \
        } while (0)
#else
#    define PYYJSON_TRACE_OP(x) (void)0
#endif


bool _pyyjson_decode_obj_stack_resize(DecodeObjStackInfo *restrict decode_obj_stack_info);

force_inline bool pyyjson_push_obj(DecodeObjStackInfo *restrict decode_obj_stack_info, PyObject *obj) {
    static_assert(((Py_ssize_t)PYYJSON_DECODE_OBJ_BUFFER_INIT_SIZE << 1) > 0, "(PYYJSON_DECODE_OBJSTACK_BUFFER_SIZE << 1) > 0");
    if (unlikely(decode_obj_stack_info->cur_write_result_addr >= decode_obj_stack_info->result_stack_end)) {
        bool c = _pyyjson_decode_obj_stack_resize(decode_obj_stack_info);
        RETURN_ON_UNLIKELY_ERR(!c);
    }
    *decode_obj_stack_info->cur_write_result_addr++ = obj;
    return true;
}

force_inline bool pyyjson_decode_arr(DecodeObjStackInfo *restrict decode_obj_stack_info, Py_ssize_t arr_len) {
    assert(arr_len >= 0);
    PyObject *list = PyList_New(arr_len);
    RETURN_ON_UNLIKELY_ERR(!list);
    PyObject **list_val_start = decode_obj_stack_info->cur_write_result_addr - arr_len;
    assert(list_val_start >= decode_obj_stack_info->result_stack);
    for (Py_ssize_t j = 0; j < arr_len; j++) {
        PyObject *val = list_val_start[j];
        assert(val);
        PyList_SET_ITEM(list, j, val); // this never fails
    }
    decode_obj_stack_info->cur_write_result_addr -= arr_len;
    return pyyjson_push_obj(decode_obj_stack_info, list);
}

force_inline bool pyyjson_decode_obj(DecodeObjStackInfo *restrict decode_obj_stack_info, Py_ssize_t dict_len) {
    PyObject *dict = _PyDict_NewPresized(dict_len);
    RETURN_ON_UNLIKELY_ERR(!dict);
    PyObject **dict_val_start = decode_obj_stack_info->cur_write_result_addr - dict_len * 2;
    PyObject **dict_val_view = dict_val_start;
    for (size_t j = 0; j < dict_len; j++) {
        PyObject *key = *dict_val_view++;
        assert(PyUnicode_Check(key));
        PyObject *val = *dict_val_view++;
        assert(((PyASCIIObject *)key)->hash != -1);
        int retcode = _PyDict_SetItem_KnownHash(dict, key, val, ((PyASCIIObject *)key)->hash); // this may fail
        if (likely(0 == retcode)) {
            Py_DecRef_NoCheck(key);
            Py_DecRef_NoCheck(val);
        } else {
            // we already decrefed some objects, have to manually handle all refcnt here
            Py_DECREF(dict);
            // also need to clean up the rest k-v pairs
            for (size_t k = j * 2; k < dict_len * 2; k++) {
                Py_DECREF(dict_val_start[k]);
            }
            // move cur_write_result_addr to the first key addr, avoid double decref
            decode_obj_stack_info->cur_write_result_addr = dict_val_start;
            return false;
        }
    }
    decode_obj_stack_info->cur_write_result_addr -= dict_len * 2;
    return pyyjson_push_obj(decode_obj_stack_info, dict);
}

force_inline bool pyyjson_decode_null(DecodeObjStackInfo *restrict decode_obj_stack_info) {
    PYYJSON_TRACE_OP(PYYJSON_OP_CONSTANTS);
    Py_Immortal_IncRef(Py_None);
    return pyyjson_push_obj(decode_obj_stack_info, Py_None);
}

force_inline bool pyyjson_decode_false(DecodeObjStackInfo *restrict decode_obj_stack_info) {
    PYYJSON_TRACE_OP(PYYJSON_OP_CONSTANTS);
    Py_Immortal_IncRef(Py_False);
    return pyyjson_push_obj(decode_obj_stack_info, Py_False);
}

force_inline bool pyyjson_decode_true(DecodeObjStackInfo *restrict decode_obj_stack_info) {
    PYYJSON_TRACE_OP(PYYJSON_OP_CONSTANTS);
    Py_Immortal_IncRef(Py_True);
    return pyyjson_push_obj(decode_obj_stack_info, Py_True);
}

force_inline bool pyyjson_decode_nan(DecodeObjStackInfo *restrict decode_obj_stack_info, bool is_signed) {
    PYYJSON_TRACE_OP(PYYJSON_OP_NAN_INF);
    PyObject *o = PyFloat_FromDouble(is_signed ? -fabs(Py_NAN) : fabs(Py_NAN));
    RETURN_ON_UNLIKELY_ERR(!o);
    return pyyjson_push_obj(decode_obj_stack_info, o);
}

/** Character type table (generate with misc/make_tables.c) */
static const u8 char_table[256] = {
        0x44, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
        0x04, 0x05, 0x45, 0x04, 0x04, 0x45, 0x04, 0x04,
        0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
        0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
        0x01, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x20,
        0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82, 0x82,
        0x82, 0x82, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x10, 0x04, 0x00, 0x00, 0x00,
        0x00, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00,
        0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
        0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
        0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
        0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
        0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
        0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
        0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
        0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
        0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
        0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
        0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
        0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
        0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
        0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
        0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
        0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08};

/** Match a character with specified type. */
force_inline bool char_is_type(u8 c, u8 type) {
    return (char_table[c] & type) != 0;
}

/** Match a whitespace: ' ', '\\t', '\\n', '\\r'. */
force_inline bool char_is_space(u8 c) {
    return char_is_type(c, (u8)CHAR_TYPE_SPACE);
}

/** Match a whitespace or comment: ' ', '\\t', '\\n', '\\r', '/'. */
force_inline bool char_is_space_or_comment(u8 c) {
    return char_is_type(c, (u8)(CHAR_TYPE_SPACE | CHAR_TYPE_COMMENT));
}

/** Match a JSON number: '-', [0-9]. */
force_inline bool char_is_number(u8 c) {
    return char_is_type(c, (u8)CHAR_TYPE_NUMBER);
}

/** Match a JSON container: '{', '['. */
force_inline bool char_is_container(u8 c) {
    return char_is_type(c, (u8)CHAR_TYPE_CONTAINER);
}

/** Match a stop character in ASCII string: '"', '\', [0x00-0x1F,0x80-0xFF]. */
force_inline bool char_is_ascii_stop(u8 c) {
    return char_is_type(c, (u8)(CHAR_TYPE_ESC_ASCII |
                                CHAR_TYPE_NON_ASCII));
}

/** Match a line end character: '\\n', '\\r', '\0'. */
// force_inline bool char_is_line_end(u8 c) {
//     return char_is_type(c, (u8)CHAR_TYPE_LINE_END);
// }

/** Match a hexadecimal numeric character: [0-9a-fA-F]. */
// force_inline bool char_is_hex(u8 c) {
//     return char_is_type(c, (u8)CHAR_TYPE_HEX);
// }


force_inline u16 read_b2_unicode(u32 uni) {
#if PY_BIG_ENDIAN
    return ((uni & 0x1f000000) >> 18) | ((uni & 0x3f0000) >> 16);
#else
    return ((uni & 0x1f) << 6) | ((uni & 0x3f00) >> 8);
#endif
}

force_inline u16 read_b3_unicode(u32 uni) {
#if PY_BIG_ENDIAN
    return ((uni & 0x0f000000) >> 12) | ((uni & 0x3f0000) >> 10) | ((uni & 0x3f00) >> 8);
#else
    return ((uni & 0x0f) << 12) | ((uni & 0x3f00) >> 2) | ((uni & 0x3f0000) >> 16);
#endif
}

force_inline u32 read_b4_unicode(u32 uni) {
#if PY_BIG_ENDIAN
    return ((uni & 0x07000000) >> 6) | ((uni & 0x3f0000) >> 4) | ((uni & 0x3f00) >> 2) | ((uni & 0x3f));
#else
    return ((uni & 0x07) << 18) | ((uni & 0x3f00) << 4) | ((uni & 0x3f0000) >> 10) | ((uni & 0x3f000000) >> 24);
#endif
}

#include "simd/check_mask_wrap.h"

#include "simd/downgrade_wrap.h"
//
#include "decode/str/str.h"
//
#include "decode_float_wrap.inl.c"

#include "simd/write_utils_wrap.h"

#include "simd/readwrite_utils_wrap.h"
//
#include "simd/long_cvt.h"
//
#include "simd/compile_feature_check.h"

#define COMPILE_UCS_LEVEL 0
#include "decode_str.inl.c"
#undef COMPILE_UCS_LEVEL

#define COMPILE_UCS_LEVEL 1
#include "decode_str.inl.c"
#undef COMPILE_UCS_LEVEL

#define COMPILE_UCS_LEVEL 2
#include "decode_str.inl.c"
#undef COMPILE_UCS_LEVEL

#define COMPILE_UCS_LEVEL 4
#include "decode_str.inl.c"
#undef COMPILE_UCS_LEVEL

#include "decode_bytes.inl.c"

#undef COMPILE_SIMD_BITS

static int invalid_arg_checked = 0;

PyObject *SIMD_NAME_MODIFIER(pyyjson_Decode)(PyObject *self, PyObject *args, PyObject *kwargs) {
    PyObject *obj;
    PyObject *ret;
    //
    PyObject *cls = NULL, *object_hook = NULL, *parse_float = NULL, *parse_int = NULL, *parse_constant = NULL, *object_pairs_hook = NULL;
    static const char *kwlist[] = {"s", "cls", "object_hook", "parse_float", "parse_int", "parse_constant", "object_pairs_hook", NULL};
    //
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|OOOOOO", (char **)kwlist, &obj, &cls, &object_hook, &parse_float, &parse_int, &parse_constant, &object_pairs_hook)) {
        return NULL;
    }

    if (!invalid_arg_checked && (cls || object_hook || parse_float || parse_int || parse_constant || object_pairs_hook)) {
        fprintf(stderr, "Warning: some options are not supported in this version of pyyjson\n");
        invalid_arg_checked = 1;
    }

    if (PyUnicode_Check(obj)) {
        PyASCIIObject *ascii_head = PYYJSON_CAST(PyASCIIObject *, obj);
        PyUnicodeObject *in_unicode = PYYJSON_CAST(PyUnicodeObject *, obj);
        int kind = ascii_head->state.ascii ? 0 : ascii_head->state.kind;
        switch (kind) {
            case PYYJSON_STRING_TYPE_ASCII: {
                ret = pyyjson_decode_str_0(in_unicode);
                break;
            }
            case PYYJSON_STRING_TYPE_LATIN1: {
                ret = pyyjson_decode_str_1(in_unicode);
                break;
            }
            case PYYJSON_STRING_TYPE_UCS2: {
                ret = pyyjson_decode_str_2(in_unicode);
                break;
            }
            case PYYJSON_STRING_TYPE_UCS4: {
                ret = pyyjson_decode_str_4(in_unicode);
                break;
            }
            default: {
                ret = NULL;
                assert(false);
                Py_UNREACHABLE();
            }
        }
        goto done;
    }

    if (PyBytes_Check(obj)) {
        char *buffer;
        Py_ssize_t length;
        if (unlikely(0 != PyBytes_AsStringAndSize(obj, &buffer, &length))) {
            ret = NULL;
            goto done;
        }
        ret = pyyjson_decode_bytes(buffer, length);
        goto done;
    }

    if (PyByteArray_Check(obj)) {
        char *buffer = PyByteArray_AS_STRING(obj);
        Py_ssize_t length = PyByteArray_GET_SIZE(obj);
        ret = pyyjson_decode_bytes(buffer, length);
        goto done;
    }

fail:;
    ret = NULL;
    PyErr_SetString(PyExc_TypeError, "Invalid argument");

done:;
    if (unlikely(!ret && !PyErr_Occurred())) {
        PyErr_SetString(JSONDecodeError, "Failed to decode JSON: unknown error");
    }
    return ret;
}
