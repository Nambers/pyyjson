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
#if defined(_POSIX_THREADS)
#include <pthread.h>
#define TLS_KEY_TYPE pthread_key_t
#elif defined(NT_THREADS)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#define TLS_KEY_TYPE DWORD
#else
#error "Unknown thread model"
#endif

#define PYYJSON_VERSION_STRING "0.0.1"

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

#ifndef PYYJSON_READER_ESTIMATED_PRETTY_RATIO
#define PYYJSON_READER_ESTIMATED_PRETTY_RATIO 16
#endif

/*
 Init buffer size for dst buffer. Must be multiple of 64.
 Cost: PYYJSON_ENCODE_DST_BUFFER_INIT_SIZE bytes per thread.
 */
#ifndef PYYJSON_ENCODE_DST_BUFFER_INIT_SIZE
#define PYYJSON_ENCODE_DST_BUFFER_INIT_SIZE (1024)
#endif

/*
 Max nested structures for encoding.
 Cost: PYYJSON_ENCODE_MAX_RECURSION * sizeof(void*) * 2 bytes per thread.
 */
#ifndef PYYJSON_ENCODE_MAX_RECURSION
#define PYYJSON_ENCODE_MAX_RECURSION (1024)
#endif

/*
 When a character needs escape when encoding,
 the following `PYYJSON_ENCODE_ESCAPE_ONCE_BYTES`
 bytes will be processed character by character without using SIMD.
 Adjust this value if the characters that need to be escaped
 are centralized in a certain range.
 */
#ifndef PYYJSON_ENCODE_ESCAPE_ONCE_BYTES
#define PYYJSON_ENCODE_ESCAPE_ONCE_BYTES (16)
#endif

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


#endif
