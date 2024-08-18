#ifndef PYYJSON_CONFIG_H
#define PYYJSON_CONFIG_H


#ifdef _DEBUG
#undef _DEBUG
#include <Python.h>
#define _DEBUG
#else
#include <Python.h>
#endif
#include <stdint.h>
#include <stdbool.h>
#include <stdalign.h>


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

#endif
