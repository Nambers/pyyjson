#ifndef PYYJSON_CONFIG_H
#define PYYJSON_CONFIG_H


#ifdef _DEBUG
#undef _DEBUG
#include <Python.h>
#define _DEBUG
#else
#include <Python.h>
#endif
#include <stdalign.h>
#include <stdbool.h>
#include <stdint.h>


// feature checks
#if INTPTR_MAX == INT64_MAX
#define PYYJSON_64BIT
#elif INTPTR_MAX == INT32_MAX
#define PYYJSON_32BIT
#else
#error "Unsupported platform"
#endif

/* String buffer size for decoding. Default cost: 512 * 1024 = 512kb (per thread). */
#ifndef PYYJSON_STRING_BUFFER_SIZE
#define PYYJSON_STRING_BUFFER_SIZE (512 * 1024)
#endif
/* Buffer for key associative cache. Default cost: 2048 * sizeof(pyyjson_cache_type) = 16kb (per thread). */
#ifndef PYYJSON_KEY_CACHE_SIZE
#define PYYJSON_KEY_CACHE_SIZE (1 << 11)
#endif
/* Stack buffer for PyObject*. Default cost: 8 * 1024 = 8kb (per thread). */
#ifndef PYYJSON_OBJSTACK_BUFFER_SIZE
#define PYYJSON_OBJSTACK_BUFFER_SIZE (1024)
#endif

/*
 Init buffer size for dst buffer. Must be multiple of 64.
 Cost: PYYJSON_ENCODE_DST_BUFFER_INIT_SIZE bytes per thread.
 */
#ifndef PYYJSON_ENCODE_DST_BUFFER_INIT_SIZE
#define PYYJSON_ENCODE_DST_BUFFER_INIT_SIZE (4096)
#endif

/*
 Init buffer size for op buffer.
 Cost: PYYJSON_ENCODE_OP_BUFFER_INIT_SIZE * sizeof(void*) bytes per thread.
 */
#ifndef PYYJSON_ENCODE_OP_BUFFER_INIT_SIZE
#define PYYJSON_ENCODE_OP_BUFFER_INIT_SIZE (1024)
#endif

/*
 Max nested structures for encoding.
 Cost: PYYJSON_ENCODE_MAX_RECURSION * sizeof(void*) * 2 bytes per thread.
 */
#ifndef PYYJSON_ENCODE_MAX_RECURSION
#define PYYJSON_ENCODE_MAX_RECURSION (1024)
#endif

#ifndef PYYJSON_ENCODE_ADDR_DESC_BUFFER_INIT_SIZE
#define PYYJSON_ENCODE_ADDR_DESC_BUFFER_INIT_SIZE ((Py_ssize_t)1 << 14)
#endif

#ifndef PYYJSON_ENCODE_OBJ_VIEW_BUFFER_INIT_SIZE
#define PYYJSON_ENCODE_OBJ_VIEW_BUFFER_INIT_SIZE ((Py_ssize_t)1 << 14)
#endif

#ifndef PYYJSON_ENCODE_VIEW_DATA_BUFFER_INIT_SIZE
#define PYYJSON_ENCODE_VIEW_DATA_BUFFER_INIT_SIZE ((Py_ssize_t)1 << 14)
#endif

#endif
