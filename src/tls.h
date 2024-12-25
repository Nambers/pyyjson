#ifndef PYYJSON_TLS_H
#define PYYJSON_TLS_H

#include "pyyjson.h"
#include <threads.h>

/*==============================================================================
 * TLS related macros
 *============================================================================*/
#if defined(_POSIX_THREADS)
#    define PYYJSON_DECLARE_TLS_GETTER(_key, _getter_name) \
        force_inline void *_getter_name(void) {            \
            return pthread_getspecific((_key));            \
        }
#    define PYYJSON_DECLARE_TLS_SETTER(_key, _setter_name) \
        force_inline bool _setter_name(void *ptr) {        \
            return 0 == pthread_setspecific(_key, ptr);    \
        }
#else
#    define PYYJSON_DECLARE_TLS_GETTER(_key, _getter_name) \
        force_inline void *_getter_name(void) {            \
            return FlsGetValue((_key));                    \
        }
#    define PYYJSON_DECLARE_TLS_SETTER(_key, _setter_name) \
        force_inline bool _setter_name(void *ptr) {        \
            return FlsSetValue(_key, ptr);                 \
        }
#endif

/*==============================================================================
 * TLS related API
 *============================================================================*/
bool pyyjson_tls_init(void);
bool pyyjson_tls_free(void);


/*==============================================================================
 * Thread Local Encode buffer
 *============================================================================*/
/* Encode TLS buffer key. */
extern TLS_KEY_TYPE _EncodeObjStackBuffer_Key;

/* The underlying data type to be stored. */
typedef struct EncodeCtnWithIndex {
    PyObject *ctn;
    Py_ssize_t index;
} EncodeCtnWithIndex;

PYYJSON_DECLARE_TLS_GETTER(_EncodeObjStackBuffer_Key, _get_encode_obj_stack_buffer_pointer)
PYYJSON_DECLARE_TLS_SETTER(_EncodeObjStackBuffer_Key, _set_encode_obj_stack_buffer_pointer)

force_inline EncodeCtnWithIndex *get_encode_obj_stack_buffer(void) {
    void *value = _get_encode_obj_stack_buffer_pointer();
    if (unlikely(value == NULL)) {
        value = malloc(PYYJSON_ENCODE_MAX_RECURSION * sizeof(EncodeCtnWithIndex));
        if (unlikely(value == NULL)) return NULL;
        bool succ = _set_encode_obj_stack_buffer_pointer(value);
        if (unlikely(!succ)) {
            free(value);
            return NULL;
        }
    }
    return (EncodeCtnWithIndex *)value;
}

/*==============================================================================
 * Thread Local Decode buffer
 *============================================================================*/
/* Decode TLS buffer key. */
extern TLS_KEY_TYPE _DecodeCtnStackBuffer_Key;

typedef struct DecodeCtnWithSize {
    Py_ssize_t raw;
} DecodeCtnWithSize;

PYYJSON_DECLARE_TLS_GETTER(_DecodeCtnStackBuffer_Key, _get_decode_ctn_stack_buffer_pointer)
PYYJSON_DECLARE_TLS_SETTER(_DecodeCtnStackBuffer_Key, _set_decode_ctn_stack_buffer_pointer)

force_inline DecodeCtnWithSize *get_decode_ctn_stack_buffer(void) {
    void *value = _get_decode_ctn_stack_buffer_pointer();
    if (unlikely(value == NULL)) {
        value = malloc(PYYJSON_DECODE_MAX_RECURSION * sizeof(DecodeCtnWithSize));
        if (unlikely(value == NULL)) return NULL;
        bool succ = _set_decode_ctn_stack_buffer_pointer(value);
        if (unlikely(!succ)) {
            free(value);
            return NULL;
        }
    }
    return (DecodeCtnWithSize *)value;
}

/*==============================================================================
 * Thread Local Decode buffer
 *============================================================================*/
/* Decode TLS buffer key. */
extern TLS_KEY_TYPE _DecodeObjStackBuffer_Key;

// typedef struct DecodeCtnWithSize {
//     Py_ssize_t raw;
// } DecodeCtnWithSize;

PYYJSON_DECLARE_TLS_GETTER(_DecodeObjStackBuffer_Key, _get_decode_obj_stack_buffer_pointer)
PYYJSON_DECLARE_TLS_SETTER(_DecodeObjStackBuffer_Key, _set_decode_obj_stack_buffer_pointer)

force_inline PyObject **get_decode_obj_stack_buffer(void) {
    void *value = _get_decode_obj_stack_buffer_pointer();
    if (unlikely(value == NULL)) {
        value = malloc(PYYJSON_DECODE_OBJ_BUFFER_INIT_SIZE * sizeof(PyObject *));
        if (unlikely(value == NULL)) return NULL;
        bool succ = _set_decode_obj_stack_buffer_pointer(value);
        if (unlikely(!succ)) {
            free(value);
            return NULL;
        }
    }
    return (PyObject **)value;
}


#endif // PYYJSON_TLS_H
