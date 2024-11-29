/*
DRAFT utf-8 encoding.
*/


#include "encode_shared.h"
#include "simd_impl.h"

#if __AVX2__

/* read: 32 bytes, write: 48 bytes, reserve: 56 bytes */
force_inline void ucs2_encode_3bytes_utf8(const uint16_t *reader, uint8_t *writer) {
    __m256i tmp = _mm256_loadu_si256((const void *) reader);
    __m256i y[2];
    __m128i x_low = _mm256_extracti128_si256(tmp, 0);
    __m128i x_high = _mm256_extracti128_si256(tmp, 1);
    y[0] = _mm256_set_m128i(x_low, x_low);
    y[1] = _mm256_set_m128i(x_high, x_high);
    uint8_t t1[32] = {0x80, 0x80, 0,
                      0x80, 0x80, 2,
                      0x80, 0x80, 4,
                      0x80, 0x80, 6,
                      0x80, 0x80, 8,
                      0x80, 0x80, 10,
                      0x80, 0x80, 12,
                      0x80, 0x80, 14,
                      0x80, 0x80, 0x80,
                      0x80, 0x80, 0x80,
                      0x80, 0x80};
    uint8_t t2[32] = {0x80, 0, 0x80,
                      0x80, 2, 0x80,
                      0x80, 4, 0x80,
                      0x80, 6, 0x80,
                      0x80, 8, 0x80,
                      0x80, 10, 0x80,
                      0x80, 12, 0x80,
                      0x80, 14, 0x80,
                      0x80, 0x80, 0x80,
                      0x80, 0x80, 0x80,
                      0x80, 0x80};
    uint8_t t3[32] = {0, 0x80, 0x80,
                      2, 0x80, 0x80,
                      4, 0x80, 0x80,
                      6, 0x80, 0x80,
                      8, 0x80, 0x80,
                      10, 0x80, 0x80,
                      12, 0x80, 0x80,
                      14, 0x80, 0x80,
                      0x80, 0x80, 0x80,
                      0x80, 0x80, 0x80,
                      0x80, 0x80};
    uint8_t m1[32] = {
            0b11111111,
            0b00111111,
            0b00111111,
            0b11111111,
            0b00111111,
            0b00111111,
            0b11111111,
            0b00111111,
            0b00111111,
            0b11111111,
            0b00111111,
            0b00111111,
            0b11111111,
            0b00111111,
            0b00111111,
            0b11111111,
            0b00111111,
            0b00111111,
            0b11111111,
            0b00111111,
            0b00111111,
            0b11111111,
            0b00111111,
            0b00111111,
            0b00000000,
            0b00000000,
            0b00000000,
            0b00000000,
            0b00000000,
            0b00000000,
            0b00000000,
            0b00000000,
    };
    uint8_t m2[32] = {
            0b11100000,
            0b10000000,
            0b10000000,
            0b11100000,
            0b10000000,
            0b10000000,
            0b11100000,
            0b10000000,
            0b10000000,
            0b11100000,
            0b10000000,
            0b10000000,
            0b11100000,
            0b10000000,
            0b10000000,
            0b11100000,
            0b10000000,
            0b10000000,
            0b11100000,
            0b10000000,
            0b10000000,
            0b11100000,
            0b10000000,
            0b10000000,
            0b00000000,
            0b00000000,
            0b00000000,
            0b00000000,
            0b00000000,
            0b00000000,
            0b00000000,
            0b00000000,
    };
    for (int i = 0; i < 2; ++i) {
        __m256i z = y[i];
        __m256i z1, z2, z3;
        /*z1 = 00000000|00000000|abcdefgh */
        z1 = _mm256_shuffle_epi8(z, _mm256_loadu_si256(t1));
        /*z2 = gh123456|78000000 */
        z2 = _mm256_srli_epi16(z, 6);
        /*z2 = 00000000|gh123456|00000000 */
        z2 = _mm256_shuffle_epi8(z2, _mm256_loadu_si256(t2));
        /*z3 = 56780000|00000000 */
        z3 = _mm256_srli_epi16(z, 12);
        /*z3 = 56780000|00000000|00000000 */
        z3 = _mm256_shuffle_epi8(z3, _mm256_loadu_si256(t3));
        /*z = 56780000|gh123456|abcdefgh */
        z = _mm256_or_si256(z1, _mm256_or_si256(z2, z3));
        /*z = 56780000|gh123400|abcdef00 */
        z = _mm256_and_si256(z, _mm256_loadu_si256(m1));
        // 5678[mmmm]|gh1234[mm]|abcdef[mm]
        z = _mm256_or_si256(z, _mm256_loadu_si256(m2));
        _mm256_storeu_si256((void *) writer, z);
        writer += 24;
    }
}

#elif __SSSE3__

/* read: 16 bytes, write: 24 bytes, reserve: 28 bytes */
void ucs2_encode_3bytes_utf8(const uint16_t *reader, uint8_t *writer) {
    __m128i x[2];
    x[0] = _mm_loadu_si128((const __m128i_u *) reader);
    x[1] = _mm_unpackhi_epi64(x[0], x[0]);
    uint8_t t1[16] = {
            0x80,
            0x80,
            0,
            0x80,
            0x80,
            2,
            0x80,
            0x80,
            4,
            0x80,
            0x80,
            6,
            0x80,
            0x80,
            0x80,
            0x80,
    };
    uint8_t t2[16] = {
            0x80,
            0,
            0x80,
            0x80,
            2,
            0x80,
            0x80,
            4,
            0x80,
            0x80,
            6,
            0x80,
            0x80,
            0x80,
            0x80,
            0x80,
    };
    uint8_t t3[16] = {0, 0x80, 0x80,
                      2, 0x80, 0x80,
                      4, 0x80, 0x80,
                      6, 0x80, 0x80,
                      0x80, 0x80, 0x80,
                      0x80};
    uint8_t m1[16] = {
            0b11111111,
            0b00111111,
            0b00111111,
            0b11111111,
            0b00111111,
            0b00111111,
            0b11111111,
            0b00111111,
            0b00111111,
            0b11111111,
            0b00111111,
            0b00111111,
            0b00000000,
            0b00000000,
            0b00000000,
            0b00000000,
    };
    uint8_t m2[16] = {
            0b11100000,
            0b10000000,
            0b10000000,
            0b11100000,
            0b10000000,
            0b10000000,
            0b11100000,
            0b10000000,
            0b10000000,
            0b11100000,
            0b10000000,
            0b10000000,
            0b00000000,
            0b00000000,
            0b00000000,
            0b00000000,
    };
    for (int i = 0; i < 2; ++i) {
        __m128i x0 = x[i];
        __m128i x1, x2, x3;
        /*x1 = 00000000|00000000|abcdefgh */
        x1 = _mm_shuffle_epi8(x0, _mm_loadu_si128(t1));
        /*x2 = gh123456|78000000 */
        x2 = _mm_srli_epi16(x0, 6);
        /*x2 = 00000000|gh123456|00000000 */
        x2 = _mm_shuffle_epi8(x2, _mm_loadu_si128(t2));
        /*x3 = 56780000|00000000 */
        x3 = _mm_srli_epi16(x0, 12);
        /*x3 = 56780000|00000000|00000000 */
        x3 = _mm_shuffle_epi8(x3, _mm_loadu_si128(t3));
        /*x0 = 56780000|gh123456|abcdefgh */
        x0 = _mm_or_si128(x1, _mm_or_si128(x2, x3));
        /*x0 = 56780000|gh123400|abcdef00 */
        x0 = _mm_and_si128(x0, _mm_loadu_si128(m1));
        // 5678[mmmm]|gh1234[mm]|abcdef[mm]
        x0 = _mm_or_si128(x0, _mm_loadu_si128(m2));
        _mm_storeu_si128((__m128i_u *) writer, x0);
        writer += 12;
    }
}

#else

/* read: 16 bytes, write: 24 bytes, reserve: 24 bytes */
void ucs2_encode_3bytes_utf8(const uint16_t *reader, uint8_t *writer) {
    for (usize i = 0; i < 8; ++i) {
        uint16_t c = *reader;
        if (unlikely(c <= 0x7ff)) {
            break;
        }
        writer[0] = (uint8_t) (0xE0 | (c >> 12));
        writer[1] = (uint8_t) (0x80 | ((c >> 6) & 0x3F));
        writer[2] = (uint8_t) (0x80 | (c & 0x3F));
        reader++;
        writer += 3;
    }
}

#endif


#if __AVX2__

/* read: 32 bytes, write: 32 bytes, reserve: 32 bytes */
force_inline void ucs2_encode_2bytes_utf8(const uint16_t *reader, uint8_t *writer) {
    /* y1 = abcdefgh|12300000 */
    __m256i y = _mm256_loadu_si256((const void *) reader);
    /* y2 = gh123000|00000000 */
    __m256i y2 = _mm256_srli_epi16(y, 6);
    /* y3 = 00000000|abcdefgh */
    __m256i y3 = _mm256_slli_epi16(y, 8);
    /* y = gh123000|abcdefgh */
    y = _mm256_or_si256(y2, y3);
    uint16_t m1[16] = {
            0b0011111111111111,
            0b0011111111111111,
            0b0011111111111111,
            0b0011111111111111,
            0b0011111111111111,
            0b0011111111111111,
            0b0011111111111111,
            0b0011111111111111,
            0b0011111111111111,
            0b0011111111111111,
            0b0011111111111111,
            0b0011111111111111,
            0b0011111111111111,
            0b0011111111111111,
            0b0011111111111111,
            0b0011111111111111,
    };
    uint16_t m2[16] = {
            0b1000000011000000,
            0b1000000011000000,
            0b1000000011000000,
            0b1000000011000000,
            0b1000000011000000,
            0b1000000011000000,
            0b1000000011000000,
            0b1000000011000000,
            0b1000000011000000,
            0b1000000011000000,
            0b1000000011000000,
            0b1000000011000000,
            0b1000000011000000,
            0b1000000011000000,
            0b1000000011000000,
            0b1000000011000000,
    };
    /* y = gh123000|abcdef00 */
    y = _mm256_and_si256(y, _mm256_load_si256(m1));
    /* y = gh123[mmm]|abcdef[mm] */
    y = _mm256_or_si256(y, _mm256_load_si256(m2));
    _mm256_storeu_si256((void *) writer, y);
}

#else

/* read: 16 bytes, write: 16 bytes, reserve: 16 bytes */
void ucs2_encode_2bytes_utf8(const uint16_t *reader, uint8_t *writer) {
    /* x = abcdefgh|12300000 */
    __m128i x = _mm_loadu_si128((const void *) reader);
    /* x2 = gh123000|00000000 */
    __m128i x2 = _mm_srli_epi16(x, 6);
    /* x3 = 00000000|abcdefgh */
    __m128i x3 = _mm_slli_epi16(x, 8);
    /* x = gh123000|abcdefgh */
    x = _mm_or_si128(x2, x3);
    uint16_t m1[8] = {
            0b0011111111111111,
            0b0011111111111111,
            0b0011111111111111,
            0b0011111111111111,
            0b0011111111111111,
            0b0011111111111111,
            0b0011111111111111,
            0b0011111111111111,
    };
    uint16_t m2[8] = {
            0b1000000011000000,
            0b1000000011000000,
            0b1000000011000000,
            0b1000000011000000,
            0b1000000011000000,
            0b1000000011000000,
            0b1000000011000000,
            0b1000000011000000,
    };
    /* x = gh123000|abcdef00 */
    x = _mm_and_si128(x, _mm_load_si128(m1));
    /* x = gh123[mmm]|abcdef[mm] */
    x = _mm_or_si128(x, _mm_load_si128(m2));
    _mm_storeu_si128((void *) writer, x);
}

#endif

#if __AVX2__

/* read: 32 bytes, write: 16 bytes, reserve: 32 bytes */
void ucs2_encode_1bytes_utf8(const uint16_t *reader, uint8_t *writer) {
    __m256i y = _mm256_loadu_si256((const void *) reader);
    __m256i y2 = _mm256_set_m128i(_mm_undefined_si128(), _mm256_extracti128_si256(y, 1));
    y = _mm256_packs_epi16(y, y2);
    _mm256_storeu_si256((void *) writer, y);
}

#else

/* read: 16 bytes, write: 8 bytes, reserve: 16 bytes */
void ucs2_encode_1bytes_utf8(const uint16_t *reader, uint8_t *writer) {
    __m128i x = _mm_loadu_si128((const void *) reader);
    x = _mm_packs_epi16(x, x);
    _mm_storeu_si128((void *) writer, x);
}

#endif

void ucs2_encode_utf8(const uint16_t *restrict reader, uint8_t *restrict writer, Py_ssize_t read_size) {
    const usize CHECK_COUNT = SIMD_BIT_SIZE / 16;
    const uint16_t *read_end = reader + read_size;
    const uint16_t *check_end = read_end - CHECK_COUNT;
    while (reader <= check_end) {
        uint16_t first = *reader;
        SIMD_TYPE mask = get_mask(reader);
        UTF8_BytesType bytes_type = check_byte_type(first);
        switch (bytes_type) {
            case UTF8_1Byte: {
                // ASCII case
                do_reserve();
                ucs2_encode_1bytes_utf8(reader, writer);
                // TODO consider escape, handle the move ptr logic here
                continue;
            }
            case UTF8_2Bytes: {
                do_reserve();
                ucs2_encode_2bytes_utf8(reader, writer);
                break;
            }
            case UTF8_3Bytes: {
                do_reserve();
                ucs2_encode_3bytes_utf8(reader, writer);
                break;
            }
        }
        assert(bytes_type != UTF8_1Byte);
        Py_ssize_t move_size = check_mask_move_size(mask, bytes_type);
        assert(move_size);
        reader += move_size;
        writer += move_size * bytes_type;
        if (unlikely(move_size < CHECK_COUNT)) {
            // encode only one
            writer += encode_single_ucs2(*reader, writer);
            reader++;
        }
    }
    if (reader == read_end) return;
    Py_ssize_t tail_len = read_end - reader;
    // TODO
}