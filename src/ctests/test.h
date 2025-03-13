
#include "simd/mask_table.h"
#include "simd/simd_impl.h"

/* Helper macros. */

#define CHECK(_x)                   \
    do {                            \
        bool _check_result_ = (_x); \
        if (!_check_result_) {      \
            return false;           \
        }                           \
    } while (0)

#define RANDOM_FILL(_x)                        \
    do {                                       \
        fill_random_buffer(&(_x), sizeof(_x)); \
    } while (0)

#define GARBAGE_FILL(_x)                 \
    do {                                 \
        memset(&(_x), 0xfa, sizeof(_x)); \
    } while (0)

#define ZERO_FILL(_x)                 \
    do {                              \
        memset(&(_x), 0, sizeof(_x)); \
    } while (0)

/* Helper functions. */

force_inline void fill_random_buffer(void *_buffer, usize length) {
    u8 *buffer = (u8 *)_buffer;
    for (usize i = 0; i < length; i++) {
        buffer[i] = rand() & 0xff;
    }
}

/* DECLARE_TEST macro. */
#if BUILD_MULTI_LIB
#    define DECLARE_TEST(_name)  \
        bool _name##_sse2(void); \
        bool _name##_avx2(void); \
        bool _name##_avx512(void);
#else
#    define DECLARE_TEST(_name) bool _name(void);
#endif

/* Tests. */

DECLARE_TEST(test_elevate_1_2_to_128)
DECLARE_TEST(test_elevate_1_4_to_128)
DECLARE_TEST(test_elevate_2_4_to_128)
