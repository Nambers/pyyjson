// requires: READ

#include "decode.h"

#define DECODE_SRC_INFO PYYJSON_CONCAT2(DecodeSrcInfo, COMPILE_READ_UCS_LEVEL)
#define READ_TO_HEX_U16 PYYJSON_CONCAT3(read, READ_BIT_SIZE, to_hex_u16)

typedef struct DECODE_SRC_INFO {
    const _FROM_TYPE *src;
    const _FROM_TYPE *const src_start;
    const _FROM_TYPE *const src_end;
} DECODE_SRC_INFO;

/**
 Scans an escaped character sequence as a UTF-16 code unit (branchless).
 e.g. "\\u005C" should pass "005C" as `cur`.
 
 This requires the string has 4-byte zero padding.
 */
force_inline bool READ_TO_HEX_U16(const _FROM_TYPE *cur, u16 *val) {
    u16 c0, c1, c2, c3, t0, t1;
    assert(cur[0] <= 255);
    assert(cur[1] <= 255);
    assert(cur[2] <= 255);
    assert(cur[3] <= 255);
    c0 = hex_conv_table[cur[0]];
    c1 = hex_conv_table[cur[1]];
    c2 = hex_conv_table[cur[2]];
    c3 = hex_conv_table[cur[3]];
    t0 = (u16)((c0 << 8) | c2);
    t1 = (u16)((c1 << 8) | c3);
    *val = (u16)((t0 << 4) | t1);
    return ((t0 | t1) & (u16)0xF0F0) == 0;
}

#undef READ_TO_HEX_U16
#undef DECODE_SRC_INFO
