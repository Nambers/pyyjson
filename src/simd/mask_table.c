#include "pyyjson.h"

pyyjson_align(64) const u8 _TailmaskTable_8[64][64] = {
        {(u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1, (u8) -1},
        {(u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) 0, (u8) -1}};
