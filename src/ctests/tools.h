#ifndef PYYJSON_CTESTS_TOOLS_H
#define PYYJSON_CTESTS_TOOLS_H

#include "test.h"
int check_ascii_ascii(u8 *input, u8 *output, int count);
int check_ucs1_2bytes(u8 *input, u8 *output, int count);
int check_ucs1_ascii(u8 *input, u8 *output, int count);
int check_ucs2_3bytes(u16 *input, u8 *output, int count);
int check_ucs2_2bytes(u16 *input, u8 *output, int count);
int check_ucs2_ascii(u16 *input, u8 *output, int count);
int check_ucs4_4bytes(u32 *input, u8 *output, int count);
int check_ucs4_3bytes(u32 *input, u8 *output, int count);
int check_ucs4_2bytes(u32 *input, u8 *output, int count);
int check_ucs4_ascii(u32 *input, u8 *output, int count);

#endif // PYYJSON_CTESTS_TOOLS_H
