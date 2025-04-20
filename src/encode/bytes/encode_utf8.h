#ifndef PYYJSON_ENCODE_UTF8_H
#define PYYJSON_ENCODE_UTF8_H
#include "pyyjson.h"

force_inline void bytes_write_ascii(u8 **writer_addr, u8 *src, usize len) {
    write_unicode_loopx4_0_0_0(unicode_buffer_info, &src, &len);
    write_unicode_loop_0_0_0(unicode_buffer_info, &src, &len);
    if (!len) goto done;
    write_unicode_trailing_impl_0_0_0(src, len, unicode_buffer_info);
done:;
}

force_inline void bytes_write_ucs1(u8 **writer_addr, u8 *src, usize len) {
}

force_inline void bytes_write_ucs2(u8 **writer_addr, u16 *src, usize len) {
}

force_inline void bytes_write_ucs4(u8 **writer_addr, u32 *src, usize len) {
}


#endif // PYYJSON_ENCODE_UTF8_H
