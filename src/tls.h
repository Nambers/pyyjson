#ifndef PYYJSON_TLS_H
#define PYYJSON_TLS_H

#include "pyyjson.h"
#include <threads.h>


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
typedef struct CtnType {
    PyObject *ctn;
    Py_ssize_t index;
} CtnType;

force_inline void *_get_encode_obj_stack_buffer_pointer(void) {
#if defined(_POSIX_THREADS)
    return pthread_getspecific(_EncodeObjStackBuffer_Key);
#else
    return FlsGetValue(_EncodeObjStackBuffer_Key);
#endif
}

force_inline bool _set_encode_obj_stack_buffer_pointer(void *ptr) {
#if defined(_POSIX_THREADS)
    return 0 == pthread_setspecific(_EncodeObjStackBuffer_Key, ptr);
#else
    return FlsSetValue(_EncodeObjStackBuffer_Key, ptr);
#endif
}

force_inline CtnType *get_encode_obj_stack_buffer(void) {
    void *value = _get_encode_obj_stack_buffer_pointer();
    if (unlikely(value == NULL)) {
        value = malloc(PYYJSON_ENCODE_MAX_RECURSION * sizeof(CtnType));
        if (unlikely(value == NULL)) return NULL;
        bool succ = _set_encode_obj_stack_buffer_pointer(value);
        if (unlikely(!succ)) {
            free(value);
            return NULL;
        }
    }
    return (CtnType *) value;
}


#endif // PYYJSON_TLS_H
