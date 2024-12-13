#include "tls.h"
#include "assert.h"


TLS_KEY_TYPE _EncodeObjStackBuffer_Key;

void _encode_obj_stack_destructor(void *ptr) {
    if (ptr) free(ptr);
}


bool pyyjson_tls_init(void) {
    bool success = true;
#if defined(_POSIX_THREADS)
    success = success && (0 == pthread_key_create(&_EncodeObjStackBuffer_Key, _encode_obj_stack_destructor));
#else
    if (success) _EncodeObjStackBuffer_Key = FlsAlloc(_encode_obj_stack_destructor);
    if (_EncodeObjStackBuffer_Key == FLS_OUT_OF_INDEXES) success = false;
#endif
    return success;
}

bool pyyjson_tls_free(void) {
    bool success = true;
#if defined(_POSIX_THREADS)
    success = success && (0 == pthread_key_delete(_EncodeObjStackBuffer_Key));
#else
    success = success && FlsFree(_EncodeObjStackBuffer_Key);
#endif
    return success;
}
