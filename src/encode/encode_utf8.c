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

#endif