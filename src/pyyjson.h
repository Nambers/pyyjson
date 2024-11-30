#ifndef PYYJSON_H
#define PYYJSON_H


#include "pyyjson_config.h"

/*==============================================================================
 * Macros
 *============================================================================*/
/** compiler attribute check (since gcc 5.0, clang 2.9, icc 17) */
#ifndef pyyjson_has_attribute
#ifdef __has_attribute
#define pyyjson_has_attribute(x) __has_attribute(x)
#else
#define pyyjson_has_attribute(x) 0
#endif
#endif

/** compiler version (GCC) */
#ifdef __GNUC__
#define PYYJSON_GCC_VER __GNUC__
#if defined(__GNUC_PATCHLEVEL__)
#define yyjson_gcc_available(major, minor, patch) \
    ((__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__) >= (major * 10000 + minor * 100 + patch))
#else
#define yyjson_gcc_available(major, minor, patch) \
    ((__GNUC__ * 10000 + __GNUC_MINOR__ * 100) >= (major * 10000 + minor * 100 + patch))
#endif
#else
#define PYYJSON_GCC_VER 0
#define yyjson_gcc_available(major, minor, patch) 0
#endif

/** C version (STDC) */
#if defined(__STDC__) && (__STDC__ >= 1) && defined(__STDC_VERSION__)
#define PYYJSON_STDC_VER __STDC_VERSION__
#else
#define PYYJSON_STDC_VER 0
#endif

/** C++ version */
#if defined(__cplusplus)
#define PYYJSON_CPP_VER __cplusplus
#else
#define PYYJSON_CPP_VER 0
#endif

/** inline for compiler */
#ifndef pyyjson_inline
#if defined(_MSC_VER) && _MSC_VER >= 1200
#define pyyjson_inline __forceinline
#elif defined(_MSC_VER)
#define pyyjson_inline __inline
#elif pyyjson_has_attribute(always_inline) || PYYJSON_GCC_VER >= 4
#define pyyjson_inline __inline__ __attribute__((always_inline))
#elif defined(__clang__) || defined(__GNUC__)
#define pyyjson_inline __inline__
#elif defined(__cplusplus) || PYYJSON_STDC_VER >= 199901L
#define pyyjson_inline inline
#else
#define pyyjson_inline
#endif
#endif

/** noinline for compiler */
#ifndef pyyjson_noinline
#if YYJSON_MSC_VER >= 1400
#define pyyjson_noinline __declspec(noinline)
#elif pyyjson_has_attribute(noinline) || PYYJSON_GCC_VER >= 4
#define pyyjson_noinline __attribute__((noinline))
#else
#define pyyjson_noinline
#endif
#endif

/** compiler builtin check (since gcc 10.0, clang 2.6, icc 2021) */
#ifndef pyyjson_has_builtin
#ifdef __has_builtin
#define pyyjson_has_builtin(x) __has_builtin(x)
#else
#define pyyjson_has_builtin(x) 0
#endif
#endif

/** likely for compiler */
#ifndef pyyjson_likely
#if pyyjson_has_builtin(__builtin_expect) || \
        (PYYJSON_GCC_VER >= 4 && PYYJSON_GCC_VER != 5)
#define pyyjson_likely(expr) __builtin_expect(!!(expr), 1)
#else
#define pyyjson_likely(expr) (expr)
#endif
#endif

/** unlikely for compiler */
#ifndef pyyjson_unlikely
#if pyyjson_has_builtin(__builtin_expect) || \
        (PYYJSON_GCC_VER >= 4 && PYYJSON_GCC_VER != 5)
#define pyyjson_unlikely(expr) __builtin_expect(!!(expr), 0)
#else
#define pyyjson_unlikely(expr) (expr)
#endif
#endif

#define static_inline static pyyjson_inline
#define force_inline static pyyjson_inline
// #define force_inline force_noinline
#define force_noinline pyyjson_noinline
#define likely pyyjson_likely
#define unlikely pyyjson_unlikely


/** repeat utils */
#define REPEAT_2(x) x, x,
#define REPEAT_4(x) REPEAT_2(x) REPEAT_2(x)
#define REPEAT_8(x) REPEAT_4(x) REPEAT_4(x)
#define REPEAT_16(x) REPEAT_8(x) REPEAT_8(x)
#define REPEAT_32(x) REPEAT_16(x) REPEAT_16(x)
#define REPEAT_64(x) REPEAT_32(x) REPEAT_32(x)


/** align for compiler */
#ifndef pyyjson_align
#if defined(_MSC_VER) && _MSC_VER >= 1300
#define pyyjson_align(x) __declspec(align(x))
#elif pyyjson_has_attribute(aligned) || defined(__GNUC__)
#define pyyjson_align(x) __attribute__((aligned(x)))
#elif PYYJSON_CPP_VER >= 201103L
#define pyyjson_align(x) alignas(x)
#else
#define pyyjson_align(x)
#endif
#endif


/* Concat macros */
#define PYYJSON_CONCAT2_EX(a, b) a##_##b
#define PYYJSON_CONCAT2(a, b) PYYJSON_CONCAT2_EX(a, b)

#define PYYJSON_CONCAT3_EX(a, b, c) a##_##b##_##c
#define PYYJSON_CONCAT3(a, b, c) PYYJSON_CONCAT3_EX(a, b, c)

#define PYYJSON_CONCAT4_EX(a, b, c, d) a##_##b##_##c##_##d
#define PYYJSON_CONCAT4(a, b, c, d) PYYJSON_CONCAT4_EX(a, b, c, d)

#define PYYJSON_CONCAT5_EX(a, b, c, d, e) a##_##b##_##c##_##d##_##e
#define PYYJSON_CONCAT5(a, b, c, d, e) PYYJSON_CONCAT5_EX(a, b, c, d, e)

#define PYYJSON_SIMPLE_CONCAT2_EX(a, b) a##b
#define PYYJSON_SIMPLE_CONCAT2(a, b) PYYJSON_SIMPLE_CONCAT2_EX(a, b)

#define PYYJSON_SIMPLE_CONCAT3_EX(a, b, c) a##b##c
#define PYYJSON_SIMPLE_CONCAT3(a, b, c) PYYJSON_SIMPLE_CONCAT3_EX(a, b, c)


#endif // PYYJSON_H
