// // requires: READ

// #include "decode.h"
// #include "pyyjson.h"
// #include "simd/simd_impl.h"

// // #define DecodeSrcInfo PYYJSON_CONCAT2(DecodeSrcInfo, COMPILE_READ_UCS_LEVEL)
// #define read_to_hex PYYJSON_CONCAT3(read, READ_BIT_SIZE, to_hex_u16)
// // #define _read_true PYYJSON_CONCAT2(_read_true, COMPILE_READ_UCS_LEVEL)
// #define _read_false PYYJSON_CONCAT2(_read_false, COMPILE_READ_UCS_LEVEL)
// #define _read_null PYYJSON_CONCAT2(_read_null, COMPILE_READ_UCS_LEVEL)
// #define _read_inf PYYJSON_CONCAT2(_read_inf, COMPILE_READ_UCS_LEVEL)
// #define _read_nan PYYJSON_CONCAT2(_read_nan, COMPILE_READ_UCS_LEVEL)
// #define read_inf_or_nan PYYJSON_CONCAT2(read_inf_or_nan, COMPILE_READ_UCS_LEVEL)

// // typedef struct DecodeSrcInfo {
// //     const _src_t *src;
// //     const _src_t *const src_start;
// //     const _src_t *const src_end;
// // } DecodeSrcInfo;

// /**
//  This table is used to convert 4 hex character sequence to a number.
//  A valid hex character [0-9A-Fa-f] will mapped to it's raw number [0x00, 0x0F],
//  an invalid hex character will mapped to [0xF0].
//  (generate with misc/make_tables.c)
//  */
// extern const u8 hex_conv_table[256];

// /**
//  Scans an escaped character sequence as a UTF-16 code unit (branchless).
//  e.g. "\\u005C" should pass "005C" as `cur`.
 
//  This requires the string has 4-byte zero padding.
//  */
// force_inline bool read_to_hex(const _src_t *cur, u16 *val) {
//     u16 c0, c1, c2, c3, t0, t1;
//     assert(cur[0] <= U8MAX);
//     assert(cur[1] <= U8MAX);
//     assert(cur[2] <= U8MAX);
//     assert(cur[3] <= U8MAX);
//     c0 = hex_conv_table[cur[0]];
//     c1 = hex_conv_table[cur[1]];
//     c2 = hex_conv_table[cur[2]];
//     c3 = hex_conv_table[cur[3]];
//     t0 = (u16)((c0 << 8) | c2);
//     t1 = (u16)((c1 << 8) | c3);
//     *val = (u16)((t0 << 4) | t1);
//     return ((t0 | t1) & (u16)0xF0F0) == 0;
// }

// /** Read 'true' literal, '*cur' should be 't'. */
// force_inline bool _read_true(const _src_t **restrict ptr, const _src_t *restrict end) {
//     _src_t *cur = (_src_t *)*ptr;
//     pyyjson_align(sizeof(_src_t) * 4) static const _src_t t[4] = {'t', 'r', 'u', 'e'};
//     if (likely(end >= cur + 4 && memcmp(cur, t, 4 * sizeof(_src_t)) == 0)) {
//         *ptr = cur + 4;
//         return true;
//     }
//     return false;
// }

// /** Read 'false' literal, '*cur' should be 'f'. */
// force_inline bool _read_false(const _src_t **restrict ptr, const _src_t *restrict end) {
//     // the first 'f' is already checked
//     _src_t *cur = (_src_t *)*ptr;
//     pyyjson_align(sizeof(_src_t) * 4) static const _src_t t[4] = {'a', 'l', 's', 'e'};
//     if (likely(end >= cur + 4 && memcmp(cur + 1, t, 4 * sizeof(_src_t)) == 0)) {
//         *ptr = cur + 5;
//         return true;
//     }
//     return false;
// }

// /** Read 'null' literal, '*cur' should be 'n'. */
// force_inline bool _read_null(const _src_t **restrict ptr, const _src_t *restrict end) {
//     _src_t *cur = (_src_t *)*ptr;
//     pyyjson_align(sizeof(_src_t) * 4) static const _src_t t[4] = {'n', 'u', 'l', 'l'};
//     if (likely(end >= cur + 4 && memcmp(cur, t, 4 * sizeof(_src_t)) == 0)) {
//         *ptr = cur + 4;
//         return true;
//     }
//     return false;
// }

// /** Read 'Infinity' literal (ignoring case). */
// force_inline bool _read_inf(const _src_t **ptr, const _src_t *end) {
// #define read_inf_vector PYYJSON_CONCAT4(vector, a, READ_UNSIGNED_BIT_NAME, READ_BIT_SIZEx8)
// #define read_inf_vector_u PYYJSON_CONCAT4(vector, u, READ_UNSIGNED_BIT_NAME, READ_BIT_SIZEx8)
//     if (unlikely(end < *ptr + 8)) {
//         return false;
//     }
//     read_inf_vector _mask = {~(_src_t)0x20, ~(_src_t)0x20, ~(_src_t)0x20, ~(_src_t)0x20,
//                              ~(_src_t)0x20, ~(_src_t)0x20, ~(_src_t)0x20, ~(_src_t)0x20};
//     read_inf_vector _template = {'I', 'N', 'F', 'I', 'N', 'I', 'T', 'Y'};
//     // pyyjson_align(sizeof(_src_t) * 8) static const _src_t _mask[8] = {
//     //         ~(_src_t)0x20, ~(_src_t)0x20, ~(_src_t)0x20, ~(_src_t)0x20,
//     //         ~(_src_t)0x20, ~(_src_t)0x20, ~(_src_t)0x20, ~(_src_t)0x20};
//     // pyyjson_align(sizeof(_src_t) * 8) static const _src_t _template[8] = {
//     //         'I', 'N', 'F', 'I', 'N', 'I', 'T', 'Y'};
//     read_inf_vector data;
//     data = *(read_inf_vector_u *)(*ptr);
//     // memcpy(&data, *ptr, sizeof(data));
//     data = data & _mask;
//     // data = data & *(read_inf_vector *)&_mask;
//     if (likely(0 == memcmp(&data, &_template, sizeof(data)))) {
//         *ptr += 8;
//         return true;
//     }
//     return false;
// #undef read_inf_vector_u
// #undef read_inf_vector
// }

// /** Read 'NaN' literal (ignoring case). */
// force_inline bool _read_nan(const _src_t **restrict ptr, const _src_t *restrict end) {
// #define read_nan_vector PYYJSON_CONCAT4(vector, a, READ_UNSIGNED_BIT_NAME, READ_BIT_SIZEx4)
// #define read_nan_vector_u PYYJSON_CONCAT4(vector, u, READ_UNSIGNED_BIT_NAME, READ_BIT_SIZEx4)
//     if (unlikely(end < *ptr + 3)) {
//         return false;
//     }
//     // it is safe to load *end, so here we load `4 * sizeof(_src_t)` bytes
//     read_nan_vector _mask = {~(_src_t)0x20, ~(_src_t)0x20, ~(_src_t)0x20, 0};
//     // pyyjson_align(sizeof(_src_t) * 4) static const _src_t _mask[4] = {~(_src_t)0x20, ~(_src_t)0x20, ~(_src_t)0x20, 0};
//     read_nan_vector _template = {'N', 'A', 'N', 0};
//     // pyyjson_align(sizeof(_src_t) * 4) static const _src_t _template[4] = {'N', 'A', 'N', 0};
//     read_nan_vector data = *(read_nan_vector_u *)(*ptr);
//     // memcpy(&data, *ptr, sizeof(data));
//     data = data & _mask;
//     // data = data & *(read_nan_vector *)&_mask;
//     if (likely(0 == memcmp(&data, &_template, sizeof(data)))) {
//         *ptr += 3;
//         return true;
//     }
//     return false;
// #undef read_nan_vector_u
// #undef read_nan_vector
// }

// /** Read 'Infinity' or 'NaN' literal (ignoring case). */
// force_inline PyObject *read_inf_or_nan(bool sign, const _src_t **ptr, const _src_t *end) {
//     if (_read_inf(ptr, end)) {
//         return PyFloat_FromDouble(sign ? -fabs(Py_HUGE_VAL) : fabs(Py_HUGE_VAL));
//     }
//     if (_read_nan(ptr, end)) {
//         return PyFloat_FromDouble(sign ? -fabs(Py_NAN) : fabs(Py_NAN));
//     }
//     return NULL;
// }

// #undef read_inf_or_nan
// #undef _read_nan
// #undef _read_inf
// #undef _read_null
// #undef _read_false
// // #undef _read_true
// #undef read_to_hex
// // #undef DecodeSrcInfo
