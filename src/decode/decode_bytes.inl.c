
/**
 Read a JSON string.
 @param ptr The head pointer of string before '"' prefix (inout).
 @param lst JSON last position.
 @param inv Allow invalid unicode.
 @param val The string value to be written.
 @param msg The error message pointer.
 @return Whether success.
 */
force_inline PyObject* read_bytes(u8 **ptr, u8 *write_buffer, bool is_key) {
    /*
     Each unicode code point is encoded as 1 to 4 bytes in UTF-8 encoding,
     we use 4-byte mask and pattern value to validate UTF-8 byte sequence,
     this requires the input data to have 4-byte zero padding.
     ---------------------------------------------------
     1 byte
     unicode range [U+0000, U+007F]
     unicode min   [.......0]
     unicode max   [.1111111]
     bit pattern   [0.......]
     ---------------------------------------------------
     2 byte
     unicode range [U+0080, U+07FF]
     unicode min   [......10 ..000000]
     unicode max   [...11111 ..111111]
     bit require   [...xxxx. ........] (1E 00)
     bit mask      [xxx..... xx......] (E0 C0)
     bit pattern   [110..... 10......] (C0 80)
     ---------------------------------------------------
     3 byte
     unicode range [U+0800, U+FFFF]
     unicode min   [........ ..100000 ..000000]
     unicode max   [....1111 ..111111 ..111111]
     bit require   [....xxxx ..x..... ........] (0F 20 00)
     bit mask      [xxxx.... xx...... xx......] (F0 C0 C0)
     bit pattern   [1110.... 10...... 10......] (E0 80 80)
     ---------------------------------------------------
     3 byte invalid (reserved for surrogate halves)
     unicode range [U+D800, U+DFFF]
     unicode min   [....1101 ..100000 ..000000]
     unicode max   [....1101 ..111111 ..111111]
     bit mask      [....xxxx ..x..... ........] (0F 20 00)
     bit pattern   [....1101 ..1..... ........] (0D 20 00)
     ---------------------------------------------------
     4 byte
     unicode range [U+10000, U+10FFFF]
     unicode min   [........ ...10000 ..000000 ..000000]
     unicode max   [.....100 ..001111 ..111111 ..111111]
     bit require   [.....xxx ..xx.... ........ ........] (07 30 00 00)
     bit mask      [xxxxx... xx...... xx...... xx......] (F8 C0 C0 C0)
     bit pattern   [11110... 10...... 10...... 10......] (F0 80 80 80)
     ---------------------------------------------------
     */
#if PY_BIG_ENDIAN
    const u32 b1_mask = 0x80000000UL;
    const u32 b1_patt = 0x00000000UL;
    const u32 b2_mask = 0xE0C00000UL;
    const u32 b2_patt = 0xC0800000UL;
    const u32 b2_requ = 0x1E000000UL;
    const u32 b3_mask = 0xF0C0C000UL;
    const u32 b3_patt = 0xE0808000UL;
    const u32 b3_requ = 0x0F200000UL;
    const u32 b3_erro = 0x0D200000UL;
    const u32 b4_mask = 0xF8C0C0C0UL;
    const u32 b4_patt = 0xF0808080UL;
    const u32 b4_requ = 0x07300000UL;
    const u32 b4_err0 = 0x04000000UL;
    const u32 b4_err1 = 0x03300000UL;
#else
    const u32 b1_mask = 0x00000080UL;
    const u32 b1_patt = 0x00000000UL;
    const u32 b2_mask = 0x0000C0E0UL;
    const u32 b2_patt = 0x000080C0UL;
    const u32 b2_requ = 0x0000001EUL;
    const u32 b3_mask = 0x00C0C0F0UL;
    const u32 b3_patt = 0x008080E0UL;
    const u32 b3_requ = 0x0000200FUL;
    const u32 b3_erro = 0x0000200DUL;
    const u32 b4_mask = 0xC0C0C0F8UL;
    const u32 b4_patt = 0x808080F0UL;
    const u32 b4_requ = 0x00003007UL;
    const u32 b4_err0 = 0x00000004UL;
    const u32 b4_err1 = 0x00003003UL;
#endif
    
#define is_valid_seq_1(uni) ( \
    ((uni & b1_mask) == b1_patt) \
)

#define is_valid_seq_2(uni) ( \
    ((uni & b2_mask) == b2_patt) && \
    ((uni & b2_requ)) \
)
    
#define is_valid_seq_3(uni) ( \
    ((uni & b3_mask) == b3_patt) && \
    ((tmp = (uni & b3_requ))) && \
    ((tmp != b3_erro)) \
)
    
#define is_valid_seq_4(uni) ( \
    ((uni & b4_mask) == b4_patt) && \
    ((tmp = (uni & b4_requ))) && \
    ((tmp & b4_err0) == 0 || (tmp & b4_err1) == 0) \
)

#define return_err(_end, _msg)                                                        \
    do {                                                                              \
        PyErr_Format(JSONDecodeError, "%s, at position %zu", _msg, _end - src_start); \
        return NULL;                                                                 \
    } while (0)

    u8 *cur = (u8 *)*ptr;
    // u8 **end = (u8 **)ptr;
    /* modified BEGIN */
    // u8 *src = ++cur, *dst, *pos;
    u8 *src = ++cur, *pos;
    /* modified END */
    u16 hi, lo;
    u32 uni, tmp;
    /* modified BEGIN */
    u8* const src_start = src;
    size_t len_ucs1 = 0, len_ucs2 = 0, len_ucs4 = 0;
    u8* temp_string_buf = write_buffer;//(u8*)*buffer;
    u8* dst = temp_string_buf;
    u8 cur_max_ucs_size = 1;
    u16* dst_ucs2;
    u32* dst_ucs4;
    bool is_ascii = true;
    /* modified END */

skip_ascii:
    /* Most strings have no escaped characters, so we can jump them quickly. */
    
skip_ascii_begin:
    /*
     We want to make loop unrolling, as shown in the following code. Some
     compiler may not generate instructions as expected, so we rewrite it with
     explicit goto statements. We hope the compiler can generate instructions
     like this: https://godbolt.org/z/8vjsYq
     
         while (true) repeat16({
            if (likely(!(char_is_ascii_stop(*src)))) src++;
            else break;
         })
     */
#define expr_jump(i) \
    if (likely(!char_is_ascii_stop(src[i]))) {} \
    else goto skip_ascii_stop##i;
    
#define expr_stop(i) \
    skip_ascii_stop##i: \
    src += i; \
    goto skip_ascii_end;
    
    REPEAT_INCR_16(expr_jump)
    src += 16;
    goto skip_ascii_begin;
    REPEAT_INCR_16(expr_stop)
    
#undef expr_jump
#undef expr_stop
    
skip_ascii_end:
    
    /*
     GCC may store src[i] in a register at each line of expr_jump(i) above.
     These instructions are useless and will degrade performance.
     This inline asm is a hint for gcc: "the memory has been modified,
     do not cache it".
     
     MSVC, Clang, ICC can generate expected instructions without this hint.
     */
#if PYYJSON_IS_REAL_GCC
    __asm__ volatile("":"=m"(*src));
#endif
    if (likely(*src == '"')) {
        /* modified BEGIN */
        // this is a fast path for ascii strings. directly copy the buffer to pyobject
        *ptr = src + 1;
        return make_string(src_start, src - src_start, PYYJSON_STRING_TYPE_ASCII, is_key);
    } else if(src != src_start){
        memcpy(temp_string_buf, src_start, src - src_start);
        len_ucs1 = src - src_start;
        dst += len_ucs1;
    }
    goto copy_utf8_ucs1;
    /* modified END */

    /* modified BEGIN */    
// skip_utf8:
//     if (*src & 0x80) { /* non-ASCII character */
//         /*
//          Non-ASCII character appears here, which means that the text is likely
//          to be written in non-English or emoticons. According to some common
//          data set statistics, byte sequences of the same length may appear
//          consecutively. We process the byte sequences of the same length in each
//          loop, which is more friendly to branch prediction.
//          */
//         pos = src;

//         while (true) repeat8({
//             if (likely((*src & 0xF0) == 0xE0)) src += 3;
//             else break;
//         })
//         if (*src < 0x80) goto skip_ascii;
//         while (true) repeat8({
//             if (likely((*src & 0xE0) == 0xC0)) src += 2;
//             else break;
//         })
//         while (true) repeat8({
//             if (likely((*src & 0xF8) == 0xF0)) src += 4;
//             else break;
//         })

//         if (unlikely(pos == src)) {
//             if (!inv) return_err(src, "invalid UTF-8 encoding in string");
//             ++src;
//         }
//         goto skip_ascii;
//     }
    
    /* The escape character appears, we need to copy it. */
    // dst = src;
    /* modified END */

    /* modified BEGIN */
copy_escape_ucs1:
    if (likely(*src == '\\')) {
    /* modified END */
        switch (*++src) {
            case '"':  *dst++ = '"';  src++; break;
            case '\\': *dst++ = '\\'; src++; break;
            case '/':  *dst++ = '/';  src++; break;
            case 'b':  *dst++ = '\b'; src++; break;
            case 'f':  *dst++ = '\f'; src++; break;
            case 'n':  *dst++ = '\n'; src++; break;
            case 'r':  *dst++ = '\r'; src++; break;
            case 't':  *dst++ = '\t'; src++; break;
            case 'u':
                if (unlikely(!read_hex_u16(++src, &hi))) {
                    return_err(src - 2, "invalid escaped sequence in string");
                }
                src += 4;
                if (likely((hi & 0xF800) != 0xD800)) {
                    /* a BMP character */
                    /* modified BEGIN */
                    if (hi >= 0x100) {
                        // BEGIN ucs1 -> ucs2
                        assert(cur_max_ucs_size == 1);
                        len_ucs1 = dst - (u8*)temp_string_buf;
                        dst_ucs2 = ((u16*)temp_string_buf) + len_ucs1;
                        cur_max_ucs_size = 2;
                        // END ucs1 -> ucs2
                        *dst_ucs2++ = hi;
                        goto copy_ascii_ucs2;
                    } else {
                        if (hi >= 0x80) is_ascii = false;  // latin1
                        *dst++ = (u8)hi;
                        goto copy_ascii_ucs1;
                    }
                    // if (hi >= 0x800) {
                    //     *dst++ = (u8)(0xE0 | (hi >> 12));
                    //     *dst++ = (u8)(0x80 | ((hi >> 6) & 0x3F));
                    //     *dst++ = (u8)(0x80 | (hi & 0x3F));
                    // } else if (hi >= 0x80) {
                    //     *dst++ = (u8)(0xC0 | (hi >> 6));
                    //     *dst++ = (u8)(0x80 | (hi & 0x3F));
                    // } else {
                    //     *dst++ = (u8)hi;
                    // }
                    /* modified END */
                } else {
                    /* a non-BMP character, represented as a surrogate pair */
                    if (unlikely((hi & 0xFC00) != 0xD800)) {
                        return_err(src - 6, "invalid high surrogate in string");
                    }
                    if (unlikely(!byte_match_2(src, "\\u"))) {
                        return_err(src, "no low surrogate in string");
                    }
                    if (unlikely(!read_hex_u16(src + 2, &lo))) {
                        return_err(src, "invalid escaped sequence in string");
                    }
                    if (unlikely((lo & 0xFC00) != 0xDC00)) {
                        return_err(src, "invalid low surrogate in string");
                    }
                    uni = ((((u32)hi - 0xD800) << 10) |
                            ((u32)lo - 0xDC00)) + 0x10000;
                    /* modified BEGIN */
                    // BEGIN ucs1 -> ucs4
                    assert(cur_max_ucs_size == 1);
                    len_ucs1 = dst - (u8*)temp_string_buf;
                    dst_ucs4 = ((u32*)temp_string_buf) + len_ucs1;
                    cur_max_ucs_size = 4;
                    // END ucs1 -> ucs4
                    *dst_ucs4++ = uni;
                    // *dst++ = (u8)(0xF0 | (uni >> 18));
                    // *dst++ = (u8)(0x80 | ((uni >> 12) & 0x3F));
                    // *dst++ = (u8)(0x80 | ((uni >> 6) & 0x3F));
                    // *dst++ = (u8)(0x80 | (uni & 0x3F));
                    src += 6;
                    goto copy_ascii_ucs4;
                    /* modified END */
                }
                break;
            default: return_err(src, "invalid escaped character in string");
        }
        /* modified BEGIN */
        goto copy_ascii_ucs1;
        /* modified END */
        /* modified BEGIN */
    } else if (likely(*src == '"')) {
        goto read_finalize;
        
        /* modified END */
    } else {
        /* modified BEGIN */
        return_err(src, "unexpected control character in string");
        // if (!inv) return_err(src, "unexpected control character in string");
        // if (src >= lst) return_err(src, "unclosed string");
        // *dst++ = *src++;
        /* modified END */
    }
    assert(false);
    Py_UNREACHABLE();

    /* modified BEGIN */
copy_ascii_ucs1:
    /* modified END */
    /*
     Copy continuous ASCII, loop unrolling, same as the following code:
     
         while (true) repeat16({
            if (unlikely(char_is_ascii_stop(*src))) break;
            *dst++ = *src++;
         })
     */
#if PYYJSON_IS_REAL_GCC
    /* modified BEGIN */
#   define expr_jump(i) \
    if (likely(!(char_is_ascii_stop(src[i])))) {} \
    else { __asm__ volatile("":"=m"(src[i])); goto copy_ascii_ucs1_stop_##i; }
    /* modified END */
#else
    /* modified BEGIN */
#   define expr_jump(i) \
    if (likely(!(char_is_ascii_stop(src[i])))) {} \
    else { goto copy_ascii_ucs1_stop_##i; }
    /* modified END */
#endif
    REPEAT_INCR_16(expr_jump)
#undef expr_jump
    
    byte_move_16(dst, src);
    src += 16;
    dst += 16;
    /* modified BEGIN */
    goto copy_ascii_ucs1;
    /* modified END */
    
    /*
     The memory will be moved forward by at least 1 byte. So the `byte_move`
     can be one byte more than needed to reduce the number of instructions.
     */
    /* modified BEGIN */
copy_ascii_ucs1_stop_0:
    goto copy_utf8_ucs1;
copy_ascii_ucs1_stop_1:
    /* modified END */
    byte_move_2(dst, src);
    src += 1;
    dst += 1;
    /* modified BEGIN */
    goto copy_utf8_ucs1;
copy_ascii_ucs1_stop_2:
    /* modified END */
    byte_move_2(dst, src);
    src += 2;
    dst += 2;
    /* modified BEGIN */
    goto copy_utf8_ucs1;
copy_ascii_ucs1_stop_3:
    /* modified END */
    byte_move_4(dst, src);
    src += 3;
    dst += 3;
    /* modified BEGIN */
    goto copy_utf8_ucs1;
copy_ascii_ucs1_stop_4:
    /* modified END */
    byte_move_4(dst, src);
    src += 4;
    dst += 4;
    /* modified BEGIN */
    goto copy_utf8_ucs1;
copy_ascii_ucs1_stop_5:
    /* modified END */
    byte_move_4(dst, src);
    byte_move_2(dst + 4, src + 4);
    src += 5;
    dst += 5;
    /* modified BEGIN */
    goto copy_utf8_ucs1;
copy_ascii_ucs1_stop_6:
    /* modified END */
    byte_move_4(dst, src);
    byte_move_2(dst + 4, src + 4);
    src += 6;
    dst += 6;
    /* modified BEGIN */
    goto copy_utf8_ucs1;
copy_ascii_ucs1_stop_7:
    /* modified END */
    byte_move_8(dst, src);
    src += 7;
    dst += 7;
    /* modified BEGIN */
    goto copy_utf8_ucs1;
copy_ascii_ucs1_stop_8:
    /* modified END */
    byte_move_8(dst, src);
    src += 8;
    dst += 8;
    /* modified BEGIN */
    goto copy_utf8_ucs1;
copy_ascii_ucs1_stop_9:
    /* modified END */
    byte_move_8(dst, src);
    byte_move_2(dst + 8, src + 8);
    src += 9;
    dst += 9;
    /* modified BEGIN */
    goto copy_utf8_ucs1;
copy_ascii_ucs1_stop_10:
    /* modified END */
    byte_move_8(dst, src);
    byte_move_2(dst + 8, src + 8);
    src += 10;
    dst += 10;
    /* modified BEGIN */
    goto copy_utf8_ucs1;
copy_ascii_ucs1_stop_11:
    /* modified END */
    byte_move_8(dst, src);
    byte_move_4(dst + 8, src + 8);
    src += 11;
    dst += 11;
    /* modified BEGIN */
    goto copy_utf8_ucs1;
copy_ascii_ucs1_stop_12:
    /* modified END */
    byte_move_8(dst, src);
    byte_move_4(dst + 8, src + 8);
    src += 12;
    dst += 12;
    /* modified BEGIN */
    goto copy_utf8_ucs1;
copy_ascii_ucs1_stop_13:
    /* modified END */
    byte_move_8(dst, src);
    byte_move_4(dst + 8, src + 8);
    byte_move_2(dst + 12, src + 12);
    src += 13;
    dst += 13;
    /* modified BEGIN */
    goto copy_utf8_ucs1;
copy_ascii_ucs1_stop_14:
    /* modified END */
    byte_move_8(dst, src);
    byte_move_4(dst + 8, src + 8);
    byte_move_2(dst + 12, src + 12);
    src += 14;
    dst += 14;
    /* modified BEGIN */
    goto copy_utf8_ucs1;
copy_ascii_ucs1_stop_15:
    /* modified END */
    byte_move_16(dst, src);
    src += 15;
    dst += 15;
    /* modified BEGIN */
    goto copy_utf8_ucs1;
    /* modified END */
    



    /* modified BEGIN */
copy_utf8_ucs1:
    assert(cur_max_ucs_size==1);
    /* modified END */
    if (*src & 0x80) { /* non-ASCII character */
copy_utf8_inner_ucs1:
        pos = src;
        uni = byte_load_4(src);
        // TODO remove the repeat4 later
        while (true) REPEAT_CALL_4({
            if ((uni & b3_mask) == b3_patt) {
                /* modified BEGIN */
                // code point: [U+0800, U+FFFF]
                // BEGIN ucs1 -> ucs2
                assert(cur_max_ucs_size == 1);
                len_ucs1 = dst - (u8*)temp_string_buf;
                dst_ucs2 = ((u16*)temp_string_buf) + len_ucs1;
                cur_max_ucs_size = 2;
                // END ucs1 -> ucs2
                // write
                *dst_ucs2++ = read_b3_unicode(uni);
                // byte_copy_4(dst, &uni);
                // dst += 3;
                // move src and load
                src += 3;
                goto copy_utf8_inner_ucs2;
                // uni = byte_load_4(src);
                /* modified END */
            } else break;
        })
        if ((uni & b1_mask) == b1_patt) goto copy_ascii_ucs1;
        while (true) REPEAT_CALL_4({
            if ((uni & b2_mask) == b2_patt) {
                /* modified BEGIN */
                assert(cur_max_ucs_size == 1);
                u16 to_write = read_b2_unicode(uni);
                if (likely(to_write >= 0x100)) {
                    // UCS2
                    // BEGIN ucs1 -> ucs2
                    assert(cur_max_ucs_size == 1);
                    len_ucs1 = dst - (u8*)temp_string_buf;
                    dst_ucs2 = ((u16*)temp_string_buf) + len_ucs1;
                    cur_max_ucs_size = 2;
                    // END ucs1 -> ucs2
                    // write
                    *dst_ucs2++ = to_write;
                    // move src and load
                    src += 2;
                    goto copy_utf8_inner_ucs2;
                    // uni = byte_load_4(src);
                } else {
                    is_ascii = false;
                    // write
                    *dst++ = (u8)to_write;
                    // move src and load
                    src += 2;
                    // still ascii, no need goto
                    uni = byte_load_4(src);
                }
                // code point: [U+0080, U+07FF], latin1 or ucs2
                // byte_copy_2(dst, &uni);
                // dst += 2;
                /* modified END */
            } else break;
        })
        while (true) REPEAT_CALL_4({
            if ((uni & b4_mask) == b4_patt) {
                /* modified BEGIN */
                // code point: [U+10000, U+10FFFF]
                // must be ucs4
                // BEGIN ucs1 -> ucs4
                assert(cur_max_ucs_size == 1);
                len_ucs1 = dst - (u8*)temp_string_buf;
                dst_ucs4 = ((u32*)temp_string_buf) + len_ucs1;
                cur_max_ucs_size = 4;
                // END ucs1 -> ucs4
                *dst_ucs4++ = read_b4_unicode(uni);
                // byte_copy_4(dst, &uni);
                // dst += 4;
                src += 4;
                goto copy_utf8_inner_ucs4;
                // uni = byte_load_4(src);
                /* modified END */
            } else break;
        })

        /* modified BEGIN */
        if (unlikely(pos == src)) {
            return_err(src, "invalid UTF-8 encoding in string");
            // goto copy_ascii_stop_1;
        }
        goto copy_ascii_ucs1;
        /* modified END */
    }
    /* modified BEGIN */
    goto copy_escape_ucs1;
    /* modified END */
    

    /* modified BEGIN */
copy_escape_ucs2:
    assert(cur_max_ucs_size==2);
    if (likely(*src == '\\')) {
    /* modified END */
        switch (*++src) {
            case '"':  *dst_ucs2++ = '"';  src++; break;
            case '\\': *dst_ucs2++ = '\\'; src++; break;
            case '/':  *dst_ucs2++ = '/';  src++; break;
            case 'b':  *dst_ucs2++ = '\b'; src++; break;
            case 'f':  *dst_ucs2++ = '\f'; src++; break;
            case 'n':  *dst_ucs2++ = '\n'; src++; break;
            case 'r':  *dst_ucs2++ = '\r'; src++; break;
            case 't':  *dst_ucs2++ = '\t'; src++; break;
            case 'u':
                if (unlikely(!read_hex_u16(++src, &hi))) {
                    return_err(src - 2, "invalid escaped sequence in string");
                }
                src += 4;
                if (likely((hi & 0xF800) != 0xD800)) {
                    /* a BMP character */
                    /* modified BEGIN */
                    *dst_ucs2++ = hi;
                    goto copy_ascii_ucs2;
                    // if (hi >= 0x800) {
                    //     *dst++ = (u8)(0xE0 | (hi >> 12));
                    //     *dst++ = (u8)(0x80 | ((hi >> 6) & 0x3F));
                    //     *dst++ = (u8)(0x80 | (hi & 0x3F));
                    // } else if (hi >= 0x80) {
                    //     *dst++ = (u8)(0xC0 | (hi >> 6));
                    //     *dst++ = (u8)(0x80 | (hi & 0x3F));
                    // } else {
                    //     *dst++ = (u8)hi;
                    // }
                    /* modified END */
                } else {
                    /* a non-BMP character, represented as a surrogate pair */
                    if (unlikely((hi & 0xFC00) != 0xD800)) {
                        return_err(src - 6, "invalid high surrogate in string");
                    }
                    if (unlikely(!byte_match_2(src, "\\u"))) {
                        return_err(src, "no low surrogate in string");
                    }
                    if (unlikely(!read_hex_u16(src + 2, &lo))) {
                        return_err(src, "invalid escaped sequence in string");
                    }
                    if (unlikely((lo & 0xFC00) != 0xDC00)) {
                        return_err(src, "invalid low surrogate in string");
                    }
                    uni = ((((u32)hi - 0xD800) << 10) |
                            ((u32)lo - 0xDC00)) + 0x10000;
                    /* modified BEGIN */
                    // BEGIN ucs2 -> ucs4
                    assert(cur_max_ucs_size == 2);
                    len_ucs2 = dst_ucs2 - (u16*)temp_string_buf - len_ucs1;
                    dst_ucs4 = ((u32*)temp_string_buf) + len_ucs1 + len_ucs2;
                    cur_max_ucs_size = 4;
                    // END ucs2 -> ucs4
                    *dst_ucs4++ = uni;
                    // *dst++ = (u8)(0xF0 | (uni >> 18));
                    // *dst++ = (u8)(0x80 | ((uni >> 12) & 0x3F));
                    // *dst++ = (u8)(0x80 | ((uni >> 6) & 0x3F));
                    // *dst++ = (u8)(0x80 | (uni & 0x3F));
                    src += 6;
                    goto copy_ascii_ucs4;
                    /* modified END */
                }
                break;
            default: return_err(src, "invalid escaped character in string");
        }
        /* modified BEGIN */
        goto copy_ascii_ucs2;
        /* modified END */
        /* modified BEGIN */
    } else if (likely(*src == '"')) {
        goto read_finalize;
        
        /* modified END */
    } else {
        /* modified BEGIN */
        return_err(src, "unexpected control character in string");
        // if (!inv) return_err(src, "unexpected control character in string");
        // if (src >= lst) return_err(src, "unclosed string");
        // *dst++ = *src++;
        /* modified END */
    }
    assert(false);
    Py_UNREACHABLE();

    /* modified BEGIN */
copy_ascii_ucs2:
    assert(cur_max_ucs_size==2);
    while (true) REPEAT_CALL_16({
        if (unlikely(char_is_ascii_stop(*src))) break;
        *dst_ucs2++ = *src++;
    })
    /* modified END */




    /* modified BEGIN */
copy_utf8_ucs2:
    assert(cur_max_ucs_size==2);
    /* modified END */
    if (*src & 0x80) { /* non-ASCII character */
copy_utf8_inner_ucs2:
        pos = src;
        uni = byte_load_4(src);
        // TODO remove the repeat4 later
        while (true) REPEAT_CALL_4({
            if ((uni & b3_mask) == b3_patt) {
                /* modified BEGIN */
                // code point: [U+0800, U+FFFF]
                assert(cur_max_ucs_size == 2);
                // write
                *dst_ucs2++ = read_b3_unicode(uni);
                // byte_copy_4(dst, &uni);
                // dst += 3;
                // move src and load
                src += 3;
                // goto copy_utf8_ucs2;
                uni = byte_load_4(src);
                /* modified END */
            } else break;
        })
        if ((uni & b1_mask) == b1_patt) goto copy_ascii_ucs2;
        while (true) REPEAT_CALL_4({
            if ((uni & b2_mask) == b2_patt) {
                /* modified BEGIN */
                assert(cur_max_ucs_size == 2);
                u16 to_write = read_b2_unicode(uni);
                *dst_ucs2++ = to_write;
                src += 2;
                uni = byte_load_4(src);
                // if (likely(to_write >= 0x100)) {
                //     // UCS2
                //     // write
                //     *dst_ucs2++ = to_write;
                //     // move src and load
                //     src += 2;
                //     goto copy_utf8_ucs2;
                //     // uni = byte_load_4(src);
                // } else {
                //     // write
                //     *dst_ucs2++ = (u8)to_write;
                //     // move src and load
                //     src += 2;
                //     // still ascii, no need goto
                //     uni = byte_load_4(src);
                // }
                // code point: [U+0080, U+07FF], latin1 or ucs2
                // byte_copy_2(dst, &uni);
                // dst += 2;
                /* modified END */
            } else break;
        })
        while (true) REPEAT_CALL_4({
            if ((uni & b4_mask) == b4_patt) {
                /* modified BEGIN */
                // code point: [U+10000, U+10FFFF]
                // must be ucs4
                // BEGIN ucs2 -> ucs4
                assert(cur_max_ucs_size == 2);
                len_ucs2 = dst_ucs2 - (u16*)temp_string_buf - len_ucs1;
                dst_ucs4 = ((u32*)temp_string_buf) + len_ucs1 + len_ucs2;
                cur_max_ucs_size = 4;
                // END ucs2 -> ucs4
                *dst_ucs4++ = read_b4_unicode(uni);
                // byte_copy_4(dst, &uni);
                // dst += 4;
                src += 4;
                goto copy_utf8_inner_ucs4;
                // uni = byte_load_4(src);
                /* modified END */
            } else break;
        })

        /* modified BEGIN */
        if (unlikely(pos == src)) {
            return_err(src, "invalid UTF-8 encoding in string");
            // goto copy_ascii_stop_1;
        }
        goto copy_ascii_ucs2;
        /* modified END */
    }
    /* modified BEGIN */
    goto copy_escape_ucs2;
    /* modified END */
    

    /* modified BEGIN */
copy_escape_ucs4:
    assert(cur_max_ucs_size==4);
    if (likely(*src == '\\')) {
    /* modified END */
        switch (*++src) {
            case '"':  *dst_ucs4++ = '"';  src++; break;
            case '\\': *dst_ucs4++ = '\\'; src++; break;
            case '/':  *dst_ucs4++ = '/';  src++; break;
            case 'b':  *dst_ucs4++ = '\b'; src++; break;
            case 'f':  *dst_ucs4++ = '\f'; src++; break;
            case 'n':  *dst_ucs4++ = '\n'; src++; break;
            case 'r':  *dst_ucs4++ = '\r'; src++; break;
            case 't':  *dst_ucs4++ = '\t'; src++; break;
            case 'u':
                if (unlikely(!read_hex_u16(++src, &hi))) {
                    return_err(src - 2, "invalid escaped sequence in string");
                }
                src += 4;
                if (likely((hi & 0xF800) != 0xD800)) {
                    /* a BMP character */
                    /* modified BEGIN */
                    *dst_ucs4++ = hi;
                    goto copy_ascii_ucs4;
                    // if (hi >= 0x800) {
                    //     *dst++ = (u8)(0xE0 | (hi >> 12));
                    //     *dst++ = (u8)(0x80 | ((hi >> 6) & 0x3F));
                    //     *dst++ = (u8)(0x80 | (hi & 0x3F));
                    // } else if (hi >= 0x80) {
                    //     *dst++ = (u8)(0xC0 | (hi >> 6));
                    //     *dst++ = (u8)(0x80 | (hi & 0x3F));
                    // } else {
                    //     *dst++ = (u8)hi;
                    // }
                    /* modified END */
                } else {
                    /* a non-BMP character, represented as a surrogate pair */
                    if (unlikely((hi & 0xFC00) != 0xD800)) {
                        return_err(src - 6, "invalid high surrogate in string");
                    }
                    if (unlikely(!byte_match_2(src, "\\u"))) {
                        return_err(src, "no low surrogate in string");
                    }
                    if (unlikely(!read_hex_u16(src + 2, &lo))) {
                        return_err(src, "invalid escaped sequence in string");
                    }
                    if (unlikely((lo & 0xFC00) != 0xDC00)) {
                        return_err(src, "invalid low surrogate in string");
                    }
                    uni = ((((u32)hi - 0xD800) << 10) |
                            ((u32)lo - 0xDC00)) + 0x10000;
                    /* modified BEGIN */
                    
                    // END ucs2 -> ucs4
                    *dst_ucs4++ = uni;
                    // *dst++ = (u8)(0xF0 | (uni >> 18));
                    // *dst++ = (u8)(0x80 | ((uni >> 12) & 0x3F));
                    // *dst++ = (u8)(0x80 | ((uni >> 6) & 0x3F));
                    // *dst++ = (u8)(0x80 | (uni & 0x3F));
                    src += 6;
                    goto copy_ascii_ucs4;
                    /* modified END */
                }
                break;
            default: return_err(src, "invalid escaped character in string");
        }
        /* modified BEGIN */
        goto copy_ascii_ucs4;
        /* modified END */
        /* modified BEGIN */
    } else if (likely(*src == '"')) {
        goto read_finalize;
        
        /* modified END */
    } else {
        /* modified BEGIN */
        return_err(src, "unexpected control character in string");
        // if (!inv) return_err(src, "unexpected control character in string");
        // if (src >= lst) return_err(src, "unclosed string");
        // *dst++ = *src++;
        /* modified END */
    }
    assert(false);
    Py_UNREACHABLE();

    /* modified BEGIN */
copy_ascii_ucs4:
    assert(cur_max_ucs_size==4);
    while (true) REPEAT_CALL_16({
        if (unlikely(char_is_ascii_stop(*src))) break;
        *dst_ucs4++ = *src++;
    })
    /* modified END */




    /* modified BEGIN */
copy_utf8_ucs4:
    assert(cur_max_ucs_size==4);
    /* modified END */
    if (*src & 0x80) { /* non-ASCII character */
copy_utf8_inner_ucs4:
        pos = src;
        uni = byte_load_4(src);
        // TODO remove the repeat4 later
        while (true) REPEAT_CALL_4({
            if ((uni & b3_mask) == b3_patt) {
                /* modified BEGIN */
                // code point: [U+0800, U+FFFF]
                assert(cur_max_ucs_size == 4);
                // write
                *dst_ucs4++ = read_b3_unicode(uni);
                // byte_copy_4(dst, &uni);
                // dst += 3;
                // move src and load
                src += 3;
                // goto copy_utf8_ucs2;
                uni = byte_load_4(src);
                /* modified END */
            } else break;
        })
        if ((uni & b1_mask) == b1_patt) goto copy_ascii_ucs4;
        while (true) REPEAT_CALL_4({
            if ((uni & b2_mask) == b2_patt) {
                /* modified BEGIN */
                assert(cur_max_ucs_size == 4);
                *dst_ucs4++ = read_b2_unicode(uni);
                src += 2;
                uni = byte_load_4(src);
                // if (likely(to_write >= 0x100)) {
                //     // UCS2
                //     // write
                //     *dst_ucs2++ = to_write;
                //     // move src and load
                //     src += 2;
                //     goto copy_utf8_ucs2;
                //     // uni = byte_load_4(src);
                // } else {
                //     // write
                //     *dst_ucs2++ = (u8)to_write;
                //     // move src and load
                //     src += 2;
                //     // still ascii, no need goto
                //     uni = byte_load_4(src);
                // }
                // code point: [U+0080, U+07FF], latin1 or ucs2
                // byte_copy_2(dst, &uni);
                // dst += 2;
                /* modified END */
            } else break;
        })
        while (true) REPEAT_CALL_4({
            if ((uni & b4_mask) == b4_patt) {
                /* modified BEGIN */
                // code point: [U+10000, U+10FFFF]
                // must be ucs4
                *dst_ucs4++ = read_b4_unicode(uni);
                // byte_copy_4(dst, &uni);
                // dst += 4;
                src += 4;
                // goto copy_utf8_ucs4;
                uni = byte_load_4(src);
                /* modified END */
            } else break;
        })

        /* modified BEGIN */
        if (unlikely(pos == src)) {
            return_err(src, "invalid UTF-8 encoding in string");
            // goto copy_ascii_stop_1;
        }
        goto copy_ascii_ucs4;
        /* modified END */
    }
    /* modified BEGIN */
    goto copy_escape_ucs4;
    /* modified END */
    
read_finalize:
    *ptr = src + 1;
    if(unlikely(cur_max_ucs_size==4)) {
        u32* start = (u32*)temp_string_buf + len_ucs1 + len_ucs2 - 1;
        u16* ucs2_back = (u16*)temp_string_buf + len_ucs1 + len_ucs2 - 1;
        u8* ucs1_back = (u8*)temp_string_buf + len_ucs1 - 1;
        while (len_ucs2) {
            *start-- = *ucs2_back--;
            len_ucs2--;
        }
        while (len_ucs1) {
            *start-- = *ucs1_back--;
            len_ucs1--;
        }
        return make_string(temp_string_buf, dst_ucs4 - (u32*)temp_string_buf, PYYJSON_STRING_TYPE_UCS4, is_key);
    } else if (unlikely(cur_max_ucs_size==2)) {
        u16* start = (u16*)temp_string_buf + len_ucs1 - 1;
        u8* ucs1_back = (u8*)temp_string_buf + len_ucs1 - 1;
        while (len_ucs1) {
            *start-- = *ucs1_back--;
            len_ucs1--;
        }
        return make_string(temp_string_buf, dst_ucs2 - (u16*)temp_string_buf, PYYJSON_STRING_TYPE_UCS2, is_key);
    } else {
        return make_string(temp_string_buf, dst - (u8*)temp_string_buf, is_ascii?PYYJSON_STRING_TYPE_ASCII:PYYJSON_STRING_TYPE_LATIN1, is_key);
    }

#undef return_err
#undef is_valid_seq_1
#undef is_valid_seq_2
#undef is_valid_seq_3
#undef is_valid_seq_4
}


/** Read JSON document (accept all style, but optimized for pretty). */
force_inline PyObject *read_bytes_root_pretty(const char *dat, usize len) {

    // container stack info
    DecodeCtnStackInfo _decode_ctn_info;
    DecodeCtnStackInfo* decode_ctn_info = &_decode_ctn_info;
    // object stack info
    DecodeObjStackInfo _decode_obj_stack_info;
    DecodeObjStackInfo * const decode_obj_stack_info = &_decode_obj_stack_info;
    memset(decode_ctn_info, 0, sizeof(DecodeCtnStackInfo));
    memset(decode_obj_stack_info, 0, sizeof(DecodeObjStackInfo));
    // init
    if(!init_decode_ctn_stack_info(decode_ctn_info) || !init_decode_obj_stack_info(decode_obj_stack_info)) goto failed_cleanup;
    u8 * string_buffer_head =(u8 *)pyyjson_string_buffer;
    
    //
    if (unlikely(len > ((size_t) (-1)) / 4)) {
        goto fail_alloc;
    }
    if (unlikely(4 * len > PYYJSON_STRING_BUFFER_SIZE)) {
        string_buffer_head = malloc(4 * len);
        if (!string_buffer_head) goto fail_alloc;
    }
    //
    u8 *cur = (u8 *) dat;
    u8 *end = (u8 *) dat + len;

    if (*cur++ == '{') {
        set_decode_ctn(decode_ctn_info->ctn, 0, false);
        if (*cur == '\n') cur++;
        goto obj_key_begin;
    } else {
        set_decode_ctn(decode_ctn_info->ctn, 0, true);
        if (*cur == '\n') cur++;
        goto arr_val_begin;
    }

arr_begin:
    /* save current container */
    /* create a new array value, save parent container offset */
    if(unlikely(!ctn_grow_check(decode_ctn_info))) goto fail_ctn_grow;
    set_decode_ctn(decode_ctn_info->ctn, 0, true);

    /* push the new array value as current container */
    if (*cur == '\n') cur++;

arr_val_begin:
#if PYYJSON_IS_REAL_GCC
    while (true) REPEAT_CALL_16({
        if (byte_match_2((void *) cur, "  ")) cur += 2;
        else
            break;
    })
#else
    while (true) REPEAT_CALL_16({
        if (likely(byte_match_2(cur, "  "))) cur += 2;
        else
            break;
    })
#endif

            if (*cur == '{') {
            cur++;
            goto obj_begin;
        }
    if (*cur == '[') {
        cur++;
        goto arr_begin;
    }
    if (char_is_number(*cur)) {
        PyObject* number_obj = read_number(&cur);
        if (likely(number_obj && pyyjson_push_obj(decode_obj_stack_info, number_obj))) {
            incr_decode_ctn_size(decode_ctn_info->ctn);
            goto arr_val_end;
        }
        goto fail_number;
    }
    if (*cur == '"') {
        PyObject* str_obj = read_bytes(&cur, string_buffer_head, false);
        if (likely(str_obj && pyyjson_push_obj(decode_obj_stack_info, str_obj))) {
            incr_decode_ctn_size(decode_ctn_info->ctn);
            goto arr_val_end;
        }
        goto fail_string;
    }
    if (*cur == 't') {
        if (likely(_read_true(&cur) && pyyjson_decode_true(decode_obj_stack_info))) {
            incr_decode_ctn_size(decode_ctn_info->ctn);
            goto arr_val_end;
        }
        goto fail_literal_true;
    }
    if (*cur == 'f') {
        if (likely(_read_false(&cur) && pyyjson_decode_false(decode_obj_stack_info))) {
            incr_decode_ctn_size(decode_ctn_info->ctn);
            goto arr_val_end;
        }
        goto fail_literal_false;
    }
    if (*cur == 'n') {
        if (likely(_read_null(&cur) && pyyjson_decode_null(decode_obj_stack_info))) {
            incr_decode_ctn_size(decode_ctn_info->ctn);
            goto arr_val_end;
        }
        if (likely(_read_nan(false, &cur) && pyyjson_decode_nan(decode_obj_stack_info, false))) {
            incr_decode_ctn_size(decode_ctn_info->ctn);
            goto arr_val_end;
        }
        goto fail_literal_null;
    }
    if (*cur == ']') {
        cur++;
        if (likely(get_decode_ctn_len(decode_ctn_info->ctn) == 0)) goto arr_end;
        while (*cur != ',') cur--;
        goto fail_trailing_comma;
    }
    if (char_is_space(*cur)) {
        while (char_is_space(*++cur))
            ;
        goto arr_val_begin;
    }
    if ((*cur == 'i' || *cur == 'I' || *cur == 'N')) {
        PyObject* number_obj = read_inf_or_nan(false, &cur);
        if (likely(number_obj && pyyjson_push_obj(decode_obj_stack_info, number_obj))) {
            incr_decode_ctn_size(decode_ctn_info->ctn);
            goto arr_val_end;
        }
        goto fail_character_val;
    }

    goto fail_character_val;

arr_val_end:
    if (byte_match_2((void *) cur, ",\n")) {
        cur += 2;
        goto arr_val_begin;
    }
    if (*cur == ',') {
        cur++;
        goto arr_val_begin;
    }
    if (*cur == ']') {
        cur++;
        goto arr_end;
    }
    if (char_is_space(*cur)) {
        while (char_is_space(*++cur))
            ;
        goto arr_val_end;
    }

    goto fail_character_arr_end;

arr_end:
    assert(decode_ctn_is_arr(decode_ctn_info->ctn));
    if(!pyyjson_decode_arr(decode_obj_stack_info, get_decode_ctn_len(decode_ctn_info->ctn))) goto failed_cleanup;
    /* pop parent as current container */
    if (unlikely(decode_ctn_info->ctn-- == decode_ctn_info->ctn_start)) {
        goto doc_end;
    }

    incr_decode_ctn_size(decode_ctn_info->ctn);
    if (*cur == '\n') cur++;
    if (!decode_ctn_is_arr(decode_ctn_info->ctn)) {
        goto obj_val_end;
    } else {
        goto arr_val_end;
    }

obj_begin:
    /* push container */
    if(unlikely(!ctn_grow_check(decode_ctn_info))) goto fail_ctn_grow;
    set_decode_ctn(decode_ctn_info->ctn, 0, false);
    if (*cur == '\n') cur++;

obj_key_begin:
#if PYYJSON_IS_REAL_GCC
    while (true) REPEAT_CALL_16({
        if (byte_match_2((void *) cur, "  ")) cur += 2;
        else
            break;
    })
#else
    while (true) REPEAT_CALL_16({
        if (likely(byte_match_2(cur, "  "))) cur += 2;
        else
            break;
    })
#endif
        if (likely(*cur == '"')) {
            PyObject* str_obj = read_bytes(&cur, string_buffer_head, true);
            if (likely(str_obj && pyyjson_push_obj(decode_obj_stack_info, str_obj))) {
                goto obj_key_end;
            }
            goto fail_string;
        }
    if (likely(*cur == '}')) {
        cur++;
        if (likely(get_decode_ctn_len(decode_ctn_info->ctn) == 0)) goto obj_end;
        goto fail_trailing_comma;
    }
    if (char_is_space(*cur)) {
        while (char_is_space(*++cur))
            ;
        goto obj_key_begin;
    }
    goto fail_character_obj_key;

obj_key_end:
    if (byte_match_2((void *) cur, ": ")) {
        cur += 2;
        goto obj_val_begin;
    }
    if (*cur == ':') {
        cur++;
        goto obj_val_begin;
    }
    if (char_is_space(*cur)) {
        while (char_is_space(*++cur))
            ;
        goto obj_key_end;
    }
    goto fail_character_obj_sep;

obj_val_begin:
    if (*cur == '"') {
        PyObject* str_obj = read_bytes(&cur, string_buffer_head, false);
        if (likely(str_obj && pyyjson_push_obj(decode_obj_stack_info, str_obj))) {
            incr_decode_ctn_size(decode_ctn_info->ctn);
            goto obj_val_end;
        }
        goto fail_string;
    }
    if (char_is_number(*cur)) {
        PyObject* number_obj = read_number(&cur);
        if (likely(number_obj && pyyjson_push_obj(decode_obj_stack_info, number_obj))) {
            incr_decode_ctn_size(decode_ctn_info->ctn);
            goto obj_val_end;
        }
        goto fail_number;
    }
    if (*cur == '{') {
        cur++;
        goto obj_begin;
    }
    if (*cur == '[') {
        cur++;
        goto arr_begin;
    }
    if (*cur == 't') {
        if (likely(_read_true(&cur) && pyyjson_decode_true(decode_obj_stack_info))) {
            incr_decode_ctn_size(decode_ctn_info->ctn);
            goto obj_val_end;
        }
        goto fail_literal_true;
    }
    if (*cur == 'f') {
        if (likely(_read_false(&cur) && pyyjson_decode_false(decode_obj_stack_info))) {
            incr_decode_ctn_size(decode_ctn_info->ctn);
            goto obj_val_end;
        }
        goto fail_literal_false;
    }
    if (*cur == 'n') {
        if (likely(_read_null(&cur) && pyyjson_decode_null(decode_obj_stack_info))) {
            incr_decode_ctn_size(decode_ctn_info->ctn);
            goto obj_val_end;
        }
        if (likely(_read_nan(false, &cur) && pyyjson_decode_nan(decode_obj_stack_info, false))) {
            incr_decode_ctn_size(decode_ctn_info->ctn);
            goto obj_val_end;
        }
        goto fail_literal_null;
    }
    if (char_is_space(*cur)) {
        while (char_is_space(*++cur))
            ;
        goto obj_val_begin;
    }
    if ((*cur == 'i' || *cur == 'I' || *cur == 'N')) {
        PyObject* number_obj = read_inf_or_nan(false, &cur);
        if (likely(number_obj && pyyjson_push_obj(decode_obj_stack_info, number_obj))) {
            incr_decode_ctn_size(decode_ctn_info->ctn);
            goto obj_val_end;
        }
        goto fail_character_val;
    }

    goto fail_character_val;

obj_val_end:
    if (byte_match_2((void *) cur, ",\n")) {
        cur += 2;
        goto obj_key_begin;
    }
    if (likely(*cur == ',')) {
        cur++;
        goto obj_key_begin;
    }
    if (likely(*cur == '}')) {
        cur++;
        goto obj_end;
    }
    if (char_is_space(*cur)) {
        while (char_is_space(*++cur))
            ;
        goto obj_val_end;
    }

    goto fail_character_obj_end;

obj_end:
    assert(!decode_ctn_is_arr(decode_ctn_info->ctn));
    if(unlikely(!pyyjson_decode_obj(decode_obj_stack_info, get_decode_ctn_len(decode_ctn_info->ctn)))) goto failed_cleanup;
    /* pop container */
    /* point to the next value */
    if (unlikely(decode_ctn_info->ctn-- == decode_ctn_info->ctn_start)) {
        goto doc_end;
    }
    incr_decode_ctn_size(decode_ctn_info->ctn);
    if (*cur == '\n') cur++;
    if (decode_ctn_is_arr(decode_ctn_info->ctn)) {
        goto arr_val_end;
    } else {
        goto obj_val_end;
    }

doc_end:
    /* check invalid contents after json document */
    if (unlikely(cur < end)) {
        while (char_is_space(*cur)) cur++;
        if (unlikely(cur < end)) goto fail_garbage;
    }

success:;
    PyObject *obj = *decode_obj_stack_info->result_stack;
    assert(decode_ctn_info->ctn == decode_ctn_info->ctn_start - 1);
    assert(decode_obj_stack_info->cur_write_result_addr == decode_obj_stack_info->result_stack + 1);
    assert(obj && !PyErr_Occurred());
    assert(obj->ob_refcnt == 1);
    // free string buffer
    if (unlikely(string_buffer_head != pyyjson_string_buffer)) {
        free(string_buffer_head);
    }
    // free obj stack buffer if allocated dynamically
    if (unlikely(decode_obj_stack_info->result_stack_end - decode_obj_stack_info->result_stack > PYYJSON_DECODE_OBJ_BUFFER_INIT_SIZE)) {
        free(decode_obj_stack_info->result_stack);
    }

    return obj;

#define return_err(_pos, _type, _msg)                                                               \
    do {                                                                                            \
        if (_type == JSONDecodeError) {                                                             \
            PyErr_Format(JSONDecodeError, "%s, at position %zu", _msg, ((u8 *) _pos) - (u8 *) dat); \
        } else {                                                                                    \
            PyErr_SetString(_type, _msg);                                                           \
        }                                                                                           \
        goto failed_cleanup;                                                                        \
    } while (0)

fail_string:
    return_err(cur, JSONDecodeError, "invalid string");
fail_number:
    return_err(cur, JSONDecodeError, "invalid number");
fail_alloc:
    return_err(cur, PyExc_MemoryError,
               "memory allocation failed");
fail_trailing_comma:
    return_err(cur, JSONDecodeError,
               "trailing comma is not allowed");
fail_literal_true:
    return_err(cur, JSONDecodeError,
               "invalid literal, expected a valid literal such as 'true'");
fail_literal_false:
    return_err(cur, JSONDecodeError,
               "invalid literal, expected a valid literal such as 'false'");
fail_literal_null:
    return_err(cur, JSONDecodeError,
               "invalid literal, expected a valid literal such as 'null'");
fail_character_val:
    return_err(cur, JSONDecodeError,
               "unexpected character, expected a valid JSON value");
fail_character_arr_end:
    return_err(cur, JSONDecodeError,
               "unexpected character, expected a comma or a closing bracket");
fail_character_obj_key:
    return_err(cur, JSONDecodeError,
               "unexpected character, expected a string for object key");
fail_character_obj_sep:
    return_err(cur, JSONDecodeError,
               "unexpected character, expected a colon after object key");
fail_character_obj_end:
    return_err(cur, JSONDecodeError,
               "unexpected character, expected a comma or a closing brace");
fail_comment:
    return_err(cur, JSONDecodeError,
               "unclosed multiline comment");
fail_garbage:
    return_err(cur, JSONDecodeError,
               "unexpected content after document");
fail_ctn_grow:
    return_err(cur, JSONDecodeError,
                "max recursion exceeded");

failed_cleanup:
    for (PyObject **obj_ptr = decode_obj_stack_info->result_stack; obj_ptr < decode_obj_stack_info->cur_write_result_addr; obj_ptr++) {
        Py_XDECREF(*obj_ptr);
    }
    // free string buffer
    if (unlikely(string_buffer_head != pyyjson_string_buffer)) {
        free(string_buffer_head);
    }
    // free obj stack buffer if allocated dynamically
    if (unlikely(decode_obj_stack_info->result_stack_end - decode_obj_stack_info->result_stack > PYYJSON_DECODE_OBJ_BUFFER_INIT_SIZE)) {
        free(decode_obj_stack_info->result_stack);
    }
    return NULL;
#undef return_err
}
