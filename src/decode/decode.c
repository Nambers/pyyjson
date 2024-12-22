#define XXH_INLINE_ALL
#include "decode.h"
#include "pyyjson.h"
#include "xxhash.h"
#include "decode_float.inl.h"
#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <threads.h>
#include "tls.h"

thread_local u8 pyyjson_string_buffer[PYYJSON_STRING_BUFFER_SIZE];

force_inline PyObject* read_bytes(const u8 **ptr, u8 *write_buffer, bool is_key);
force_inline PyObject *read_bytes_root_pretty(const char *dat, usize len);

force_inline bool decode_ctn_is_arr(DecodeCtnWithSize*ctn){
    return ctn->raw < 0;
}

force_inline Py_ssize_t get_decode_ctn_len(DecodeCtnWithSize*ctn){
    return ctn->raw & PY_SSIZE_T_MAX;
}

force_inline void set_decode_ctn(DecodeCtnWithSize*ctn, Py_ssize_t len, bool is_arr){
    assert(len >= 0);
    ctn->raw = len | (is_arr ? PY_SSIZE_T_MIN : 0);
}

force_inline void incr_decode_ctn_size(DecodeCtnWithSize*ctn){
    assert(ctn->raw != PY_SSIZE_T_MAX);
    ctn->raw++;
}

force_inline bool ctn_grow_check(DecodeCtnStackInfo *decode_ctn_info) {
    return ++decode_ctn_info->ctn < decode_ctn_info->ctn_end;
}

#if PY_MINOR_VERSION >= 13
// these are hidden in Python 3.13
#if PY_MINOR_VERSION == 13
PyAPI_FUNC(Py_hash_t) _Py_HashBytes(const void *, Py_ssize_t);
#endif // PY_MINOR_VERSION == 13
PyAPI_FUNC(int) _PyDict_SetItem_KnownHash(PyObject *mp, PyObject *key, PyObject *item, Py_hash_t hash);
#endif // PY_MINOR_VERSION >= 13

#if PY_MINOR_VERSION >= 12
#define PYYJSON_PY_DECREF_DEBUG() (_Py_DECREF_STAT_INC())
#define PYYJSON_PY_INCREF_DEBUG() (_Py_INCREF_STAT_INC())
#else
#ifdef Py_REF_DEBUG
#define PYYJSON_PY_DECREF_DEBUG() (_Py_RefTotal--)
#define PYYJSON_PY_INCREF_DEBUG() (_Py_RefTotal++)
#else
#define PYYJSON_PY_DECREF_DEBUG()
#define PYYJSON_PY_INCREF_DEBUG()
#endif
#endif
#define REHASHER(_x) (((size_t) (_x)) % (PYYJSON_KEY_CACHE_SIZE))

typedef XXH64_hash_t pyyjson_hash_t;


#if PYYJSON_ENABLE_TRACE
Py_ssize_t max_str_len = 0;
int __count_trace[PYYJSON_OP_BITCOUNT_MAX] = {0};
int __hash_trace[PYYJSON_KEY_CACHE_SIZE] = {0};
size_t __hash_hit_counter = 0;
size_t __hash_add_key_call_count = 0;

#define PYYJSON_TRACE_STR_LEN(_len) max_str_len = max_str_len > _len ? max_str_len : _len
#define PYYJSON_TRACE_HASH(_hash) \
    __hash_add_key_call_count++;  \
    __hash_trace[_hash & (PYYJSON_KEY_CACHE_SIZE - 1)]++
#define PYYJSON_TRACE_CACHE_HIT() __hash_hit_counter++
#define PYYJSON_TRACE_HASH_CONFLICT(_hash) printf("hash conflict: %lld, index=%lld\n", (long long int) _hash, (long long int) (_hash & (PYYJSON_KEY_CACHE_SIZE - 1)))
#else // PYYJSON_ENABLE_TRACE
#define PYYJSON_TRACE_STR_LEN(_len) (void) (0)
#define PYYJSON_TRACE_HASH(_hash) (void) (0)
#define PYYJSON_TRACE_CACHE_HIT() (void) (0)
#define PYYJSON_TRACE_HASH_CONFLICT(_hash) (void) (0)
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
#if SIZEOF_VOID_P > 4
    // Portable saturated add, branching on the carry flag and set low bits
#ifndef NDEBUG
    assert(0 > (int32_t) op->ob_refcnt_split[PY_BIG_ENDIAN]);
#endif // NDEBUG
#else  // SIZEOF_VOID_P > 4
    // Explicitly check immortality against the immortal value
    assert(_Py_IsImmortal(op));
#endif // SIZEOF_VOID_P > 4
#else  // PY_MINOR_VERSION >= 12
    op->ob_refcnt++;
#endif // PY_MINOR_VERSION >= 12
    PYYJSON_PY_INCREF_DEBUG();
}

pyyjson_cache_type AssociativeKeyCache[PYYJSON_KEY_CACHE_SIZE];

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

force_inline PyObject *get_key_cache(const u8 *utf8_str, pyyjson_hash_t hash, size_t real_len) {
    assert(real_len <= 64);
    pyyjson_cache_type cache = AssociativeKeyCache[REHASHER(hash)];
    if (!cache) return NULL;
    if (likely(((real_len == PyUnicode_GET_LENGTH(cache) * PyUnicode_KIND(cache))) && (memcmp(PyUnicode_DATA(cache), utf8_str, real_len) == 0))) {
        PYYJSON_TRACE_CACHE_HIT();
        return cache;
    }
    return NULL;
}

force_inline PyObject *make_string(const u8 *utf8_str, Py_ssize_t len, int type_flag, bool is_key) {
    PYYJSON_TRACE_STR_LEN(len);
    PyObject *obj;
    Py_UCS4 max_char;
    size_t real_len;
    XXH64_hash_t hash;

    switch (type_flag) {
        case PYYJSON_STRING_TYPE_ASCII: {
            max_char = 0x7f;
            real_len = len;
            break;
        }
        case PYYJSON_STRING_TYPE_LATIN1: {
            max_char = 0xff;
            real_len = len;
            break;
        }
        case PYYJSON_STRING_TYPE_UCS2: {
            max_char = 0xffff;
            real_len = len * 2;
            break;
        }
        case PYYJSON_STRING_TYPE_UCS4: {
            max_char = 0x10ffff;
            real_len = len * 4;
            break;
        }
        default:
            assert(false);
            Py_UNREACHABLE();
    }

    bool should_cache = (is_key && likely(real_len <= 64));

    if (should_cache) {
        hash = XXH3_64bits(utf8_str, real_len);
        obj = get_key_cache(utf8_str, hash, real_len);
        if (obj) {
            Py_INCREF(obj);
            return obj;
        }
    }

    obj = PyUnicode_New(len, max_char);
    if (obj == NULL) return NULL;
    memcpy(PyUnicode_DATA(obj), utf8_str, real_len);
    if (should_cache) {
        add_key_cache(hash, obj);
    }
success:
    if (is_key) {
        assert(((PyASCIIObject *) obj)->hash == -1);
#if PY_MINOR_VERSION >= 14
        ((PyASCIIObject *) obj)->hash = PyUnicode_Type.tp_hash(obj);
#else
        ((PyASCIIObject *) obj)->hash = _Py_HashBytes(utf8_str, real_len);
#endif
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
#define PYYJSON_TRACE_OP(x)                                 \
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
#define PYYJSON_TRACE_OP(x)    (void)0
#endif

force_noinline bool _pyyjson_decode_obj_stack_resize(DecodeObjStackInfo *restrict decode_obj_stack_info) {
    // resize
    if (likely(PYYJSON_DECODE_OBJ_BUFFER_INIT_SIZE == decode_obj_stack_info->result_stack_end - decode_obj_stack_info->result_stack)) {
        void *new_buffer = malloc(sizeof(PyObject *) * (PYYJSON_DECODE_OBJ_BUFFER_INIT_SIZE << 1));
        if (unlikely(!new_buffer)) {
            PyErr_NoMemory();
            return false;
        }
        memcpy(new_buffer, decode_obj_stack_info->result_stack, sizeof(PyObject *) * PYYJSON_DECODE_OBJ_BUFFER_INIT_SIZE);
        decode_obj_stack_info->result_stack = (PyObject **) new_buffer;
        decode_obj_stack_info->cur_write_result_addr = decode_obj_stack_info->result_stack + PYYJSON_DECODE_OBJ_BUFFER_INIT_SIZE;
        decode_obj_stack_info->result_stack_end = decode_obj_stack_info->result_stack + (PYYJSON_DECODE_OBJ_BUFFER_INIT_SIZE << 1);
    } else {
        Py_ssize_t old_capacity = decode_obj_stack_info->result_stack_end - decode_obj_stack_info->result_stack;
        if (unlikely((PY_SSIZE_T_MAX >> 1) < old_capacity)) {
            PyErr_NoMemory();
            return false;
        }
        Py_ssize_t new_capacity = old_capacity << 1;
        void *new_buffer = realloc(decode_obj_stack_info->result_stack, sizeof(PyObject *) * new_capacity);
        if (unlikely(!new_buffer)) {
            PyErr_NoMemory();
            return false;
        }
        decode_obj_stack_info->result_stack = (PyObject **) new_buffer;
        decode_obj_stack_info->cur_write_result_addr = decode_obj_stack_info->result_stack + old_capacity;
        decode_obj_stack_info->result_stack_end = decode_obj_stack_info->result_stack + new_capacity;
    }
    return true;
}

force_inline bool pyyjson_push_obj(DecodeObjStackInfo *restrict decode_obj_stack_info, PyObject *obj) {
    static_assert(((Py_ssize_t) PYYJSON_DECODE_OBJ_BUFFER_INIT_SIZE << 1) > 0, "(PYYJSON_DECODE_OBJSTACK_BUFFER_SIZE << 1) > 0");
    if (unlikely(decode_obj_stack_info->cur_write_result_addr >= decode_obj_stack_info->result_stack_end)) {
        bool c = _pyyjson_decode_obj_stack_resize(decode_obj_stack_info);
        RETURN_ON_UNLIKELY_ERR(!c);
    }
    *decode_obj_stack_info->cur_write_result_addr++ = obj;
    return true;
}

force_inline bool pyyjson_decode_string(DecodeObjStackInfo *restrict decode_obj_stack_info, const u8 *utf8_str, Py_ssize_t len, int type_flag, bool is_key) {
    PYYJSON_TRACE_OP(PYYJSON_OP_STRING);
    PyObject *new_val = make_string(utf8_str, len, type_flag, is_key);
    RETURN_ON_UNLIKELY_ERR(!new_val);
    return pyyjson_push_obj(decode_obj_stack_info, new_val);
}

force_inline bool pyyjson_decode_double(DecodeObjStackInfo* restrict decode_obj_stack_info, double val) {
    PYYJSON_TRACE_OP(PYYJSON_OP_NUMBER);
    PyObject* obj = PyFloat_FromDouble(val);
    RETURN_ON_UNLIKELY_ERR(!obj);
    return pyyjson_push_obj(decode_obj_stack_info, obj);
}

force_inline bool pyyjson_decode_longlong(DecodeObjStackInfo* restrict decode_obj_stack_info, i64 val) {
    PYYJSON_TRACE_OP(PYYJSON_OP_NUMBER);
    PyObject* obj = PyLong_FromLongLong(val);
    RETURN_ON_UNLIKELY_ERR(!obj);
    return pyyjson_push_obj(decode_obj_stack_info, obj);
}

force_inline bool pyyjson_decode_unsignedlonglong(DecodeObjStackInfo* restrict decode_obj_stack_info, u64 val) {
    PYYJSON_TRACE_OP(PYYJSON_OP_NUMBER);
    PyObject* obj = PyLong_FromUnsignedLongLong(val);
    RETURN_ON_UNLIKELY_ERR(!obj);
    return pyyjson_push_obj(decode_obj_stack_info, obj);
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
        assert(((PyASCIIObject *) key)->hash != -1);
        int retcode = _PyDict_SetItem_KnownHash(dict, key, val, ((PyASCIIObject *) key)->hash); // this may fail
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

force_inline bool pyyjson_decode_inf(DecodeObjStackInfo *restrict decode_obj_stack_info, bool is_signed) {
    PYYJSON_TRACE_OP(PYYJSON_OP_NAN_INF);
    PyObject *o = PyFloat_FromDouble(is_signed ? -fabs(Py_HUGE_VAL) : fabs(Py_HUGE_VAL));
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
    0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08
};


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
    return ((uni & 0x07000000) >> 6) | ((uni & 0x3f0000) >> 4) | ((uni & 0x3f00) >> 2)  | ((uni & 0x3f));
#else
    return ((uni & 0x07) << 18) | ((uni & 0x3f00) << 4) | ((uni & 0x3f0000) >> 10) | ((uni & 0x3f000000) >> 24);
#endif
}


/** Read single value JSON document. */
force_noinline PyObject *read_root_single(const char *dat, usize len) {
#define return_err(_pos, _type, _msg)                                                               \
    do {                                                                                            \
        if (_type == JSONDecodeError) {                                                             \
            PyErr_Format(JSONDecodeError, "%s, at position %zu", _msg, ((u8 *) _pos) - (u8 *) dat); \
        } else {                                                                                    \
            PyErr_SetString(_type, _msg);                                                           \
        }                                                                                           \
        goto fail_cleanup;                                                                                \
    } while (0)

    const u8 *cur = (const u8 *) dat;
    const u8 *const end = cur + len;

    PyObject *ret = NULL;

    if (char_is_number(*cur)) {
        ret = read_number(&cur);
        if (likely(ret)) goto single_end;
        goto fail_number;
    }
    if (*cur == '"') {
        u8 *write_buffer;
        bool dynamic = false;
        if (unlikely(4 * len > PYYJSON_STRING_BUFFER_SIZE)) {
            write_buffer = malloc(4 * len);
            if (unlikely(!write_buffer)) goto fail_alloc;
            dynamic = true;
        } else {
            write_buffer = pyyjson_string_buffer;
        }
        ret = read_bytes(&cur, write_buffer, false);
        if (dynamic) free(write_buffer);
        if (likely(ret)) goto single_end;
        goto fail_string;
    }
    if (*cur == 't') {
        if (likely(_read_true(&cur))) {
            Py_Immortal_IncRef(Py_True);
            ret = Py_True;
            goto single_end;
        }
        goto fail_literal_true;
    }
    if (*cur == 'f') {
        if (likely(_read_false(&cur))) {
            Py_Immortal_IncRef(Py_False);
            ret = Py_False;
            goto single_end;
        }
        goto fail_literal_false;
    }
    if (*cur == 'n') {
        if (likely(_read_null(&cur))) {
            Py_Immortal_IncRef(Py_None);
            ret = Py_None;
            goto single_end;
        }
        if (_read_nan(false, &cur)) {
            ret = PyFloat_FromDouble(fabs(Py_NAN));
            if(likely(ret)) goto single_end;
        }
        goto fail_literal_null;
    }
    {
        ret = read_inf_or_nan(false, &cur);
        if (likely(ret)) goto single_end;
    }
    goto fail_character;

single_end:
    assert(ret);
    if (unlikely(cur < end)) {
        while (char_is_space(*cur)) cur++;
        if (unlikely(cur < end)) goto fail_garbage;
    }
    return ret;

fail_string:
    return_err(cur, JSONDecodeError, "invalid string");
fail_number:
    return_err(cur, JSONDecodeError, "invalid number");
fail_alloc:
    return_err(cur, PyExc_MemoryError,
               "memory allocation failed");
fail_literal_true:
    return_err(cur, JSONDecodeError,
               "invalid literal, expected a valid literal such as 'true'");
fail_literal_false:
    return_err(cur, JSONDecodeError,
               "invalid literal, expected a valid literal such as 'false'");
fail_literal_null:
    return_err(cur, JSONDecodeError,
               "invalid literal, expected a valid literal such as 'null'");
fail_character:
    return_err(cur, JSONDecodeError,
               "unexpected character, expected a valid root value");
fail_comment:
    return_err(cur, JSONDecodeError,
               "unclosed multiline comment");
fail_garbage:
    return_err(cur, JSONDecodeError,
               "unexpected content after document");
fail_cleanup:
    Py_XDECREF(ret);
    return NULL;
#undef return_err
}



PyObject *yyjson_read_opts(const char *dat,
                           Py_ssize_t len
) {

#define return_err(_pos, _type, _msg)                               \
    do {                                                            \
        if (_type == JSONDecodeError) {                             \
            PyErr_Format(JSONDecodeError, "%s at %zu", _msg, _pos); \
        } else {                                                    \
            PyErr_SetString(_type, _msg);                           \
        }                                                           \
        return NULL;                                                \
    } while (0)

    PyObject *obj;
    const char *end = dat + len;

    if (unlikely(!dat)) {
        return_err(0, JSONDecodeError, "input data is NULL");
    }
    if (unlikely(!len)) {
        return_err(0, JSONDecodeError, "input length is 0");
    }

    if (unlikely(len >= USIZE_MAX)) {
        return_err(0, PyExc_MemoryError, "memory allocation failed");
    }

    /* skip empty contents before json document */
    if (unlikely(char_is_space_or_comment(*dat))) {
        if (likely(char_is_space(*dat))) {
            while (char_is_space(*++dat))
                ;
        }
        if (unlikely(dat >= end)) {
            return_err(0, JSONDecodeError, "input data is empty");
        }
    }

    /* read json document */
    // obj = read_bytes_root_pretty(dat, len);
    // TODO
    if (likely(char_is_container(*dat))) {
        if (char_is_space(dat[1]) && char_is_space(dat[2])) {
            obj = read_bytes_root_pretty(dat, len);
        } else {
            obj = read_bytes_root_pretty(dat, len);
            // obj = read_root_minify(dat, len);
        }
    } else {
        obj = read_root_single(dat, len);
    }

    /* check result */
    // if (likely(obj)) {
    //     memset(err, 0, sizeof(yyjson_read_err));
    // } else {
    //     /* RFC 8259: JSON text MUST be encoded using UTF-8 */
    //     if (err->pos == 0 && err->code != YYJSON_READ_ERROR_MEMORY_ALLOCATION) {
    //         if ((hdr[0] == 0xEF && hdr[1] == 0xBB && hdr[2] == 0xBF)) {
    //             err->msg = "byte order mark (BOM) is not supported";
    //         } else if (len >= 4 &&
    //                    ((hdr[0] == 0x00 && hdr[1] == 0x00 &&
    //                      hdr[2] == 0xFE && hdr[3] == 0xFF) ||
    //                     (hdr[0] == 0xFF && hdr[1] == 0xFE &&
    //                      hdr[2] == 0x00 && hdr[3] == 0x00))) {
    //             err->msg = "UTF-32 encoding is not supported";
    //         } else if (len >= 2 &&
    //                    ((hdr[0] == 0xFE && hdr[1] == 0xFF) ||
    //                     (hdr[0] == 0xFF && hdr[1] == 0xFE))) {
    //             err->msg = "UTF-16 encoding is not supported";
    //         }
    //     }
    //     if (!has_read_flag(INSITU)) alc.free(alc.ctx, (void *)hdr);
    // }
    return obj;

#undef return_err
}


#include "decode_bytes.inl.c"
