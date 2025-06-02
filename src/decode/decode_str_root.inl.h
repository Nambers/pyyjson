#ifdef PYYJSON_CLANGD_DUMMY
#    ifndef COMPILE_SIMD_BITS
#        include "decode.h"
#        define DECODE_READ_PRETTY 1
#        define COMPILE_SIMD_BITS 256
#        define COMPILE_READ_UCS_LEVEL 1
#        include "compile_context/sr_in.inl.h"
#    endif
#endif
/*
 * Required macros:
 *   READ_ROOT_IMPL, points to the function name
 *   DECODE_READ_PRETTY, true/false
 */

#define WRAPPED_CHAR_IS_SPACE(_u8ptr) (*_u8ptr <= U8MAX && char_is_space(*_u8ptr))
// use SKIP_CONSECUTIVE_SPACES after a `WRAPPED_CHAR_IS_SPACE` check
#define SKIP_CONSECUTIVE_SPACES(_u8ptr)          \
    do {                                         \
        do {                                     \
            _u8ptr++;                            \
        } while (WRAPPED_CHAR_IS_SPACE(_u8ptr)); \
    } while (0)

/** Read JSON document (accept all style, but optimized for pretty). */
static force_noinline PyObject *READ_ROOT_IMPL(const _src_t *dat, Py_ssize_t len) {
    static _src_t _CommaReturn[2] = {',', '\n'};
    static _src_t _CommaSpace[2] = {',', ' '};
    static _src_t _ColonSpace[2] = {':', ' '};

    const _src_t *cur = dat;
    const _src_t *const end = cur + len;
    // container stack info
    DecodeCtnStackInfo _decode_ctn_info;
    DecodeCtnStackInfo *decode_ctn_info = &_decode_ctn_info;
    // object stack info
    DecodeObjStackInfo _decode_obj_stack_info;
    DecodeObjStackInfo *const decode_obj_stack_info = &_decode_obj_stack_info;
    // process str jump flag
    int process_str_jump_flag;
    memset(decode_ctn_info, 0, sizeof(DecodeCtnStackInfo));
    memset(decode_obj_stack_info, 0, sizeof(DecodeObjStackInfo));
    // init
    if (!init_decode_ctn_stack_info(decode_ctn_info) || !init_decode_obj_stack_info(decode_obj_stack_info)) goto failed_cleanup;
    _src_t *string_buffer_head;
    bool need_dealloc = false;
    if (unlikely(!check_and_reserve_str_buffer(len, &string_buffer_head, &need_dealloc))) {
        goto fail_alloc;
    }

    if (*cur++ == '{') {
        set_decode_ctn(decode_ctn_info->ctn, 0, false);
        if (DECODE_READ_PRETTY && *cur == '\n') cur++;
        goto obj_key_begin;
    } else {
        set_decode_ctn(decode_ctn_info->ctn, 0, true);
        if (DECODE_READ_PRETTY && *cur == '\n') cur++;
        goto arr_val_begin;
    }

process_str:;
    {
        // process_str_jump_flag: 0 -> key, 1 -> obj value, 2 -> arr value
        PyObject *str_obj = decode_str(&cur, end, string_buffer_head, process_str_jump_flag == 0);
        if (likely(str_obj && pyyjson_push_obj(decode_obj_stack_info, str_obj))) {
            if (process_str_jump_flag) incr_decode_ctn_size(decode_ctn_info->ctn);
            switch (process_str_jump_flag) {
                case 0:
                    goto obj_key_end;
                case 1:
                    goto obj_val_end;
                case 2:
                    goto arr_val_end;
                default: {
                    PYYJSON_UNREACHABLE();
                }
            }
        }
        goto fail_string;
    }

arr_begin:
    /* save current container */
    /* create a new array value, save parent container offset */
    if (unlikely(!ctn_grow_check(decode_ctn_info))) goto fail_ctn_grow;
    set_decode_ctn(decode_ctn_info->ctn, 0, true);

    /* push the new array value as current container */
    if (DECODE_READ_PRETTY && *cur == '\n') cur++;

arr_val_begin:
#if DECODE_READ_PRETTY
    // assume that we jumped from arr_val_end, already skipped a dot and a return
    if (*cur == ' ') {
        // cur++;
        // if (*cur == ' ')
        fast_skip_spaces(&cur, end);
    }
    // #if PYYJSON_IS_REAL_GCC
    //     while (true) REPEAT_CALL_16({
    //         if (byte_match_2((void *)cur, "  ")) cur += 2;
    //         else
    //             break;
    //     })
    // #else
    //     while (true) REPEAT_CALL_16({
    //         if (likely(byte_match_2(cur, "  "))) cur += 2;
    //         else
    //             break;
    //     })
    // #endif
#endif

    if (*cur == '{') {
        cur++;
        goto obj_begin;
    }
    if (*cur == '[') {
        cur++;
        goto arr_begin;
    }
    if (*cur <= U8MAX && char_is_number(*cur)) {
        PyObject *number_obj = READ_NUMBER(&cur, end);
        if (likely(number_obj && pyyjson_push_obj(decode_obj_stack_info, number_obj))) {
            incr_decode_ctn_size(decode_ctn_info->ctn);
            goto arr_val_end;
        }
        goto fail_number;
    }
    if (*cur == '"') {
        cur++;
        process_str_jump_flag = 2; // arr value
        goto process_str;
    }
    if (*cur == 't') {
        if (likely(_read_true(&cur, end) && pyyjson_decode_true(decode_obj_stack_info))) {
            incr_decode_ctn_size(decode_ctn_info->ctn);
            goto arr_val_end;
        }
        goto fail_literal_true;
    }
    if (*cur == 'f') {
        if (likely(_read_false(&cur, end) && pyyjson_decode_false(decode_obj_stack_info))) {
            incr_decode_ctn_size(decode_ctn_info->ctn);
            goto arr_val_end;
        }
        goto fail_literal_false;
    }
    if (*cur == 'n') {
        if (likely(_read_null(&cur, end) && pyyjson_decode_null(decode_obj_stack_info))) {
            incr_decode_ctn_size(decode_ctn_info->ctn);
            goto arr_val_end;
        }
        if (likely(_read_nan(&cur, end) && pyyjson_decode_nan(decode_obj_stack_info, false))) {
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
    if (WRAPPED_CHAR_IS_SPACE(cur)) {
        // read pretty:
        //   the ",\n" and white spaces after them are all read out,
        //   this case is unlikely.
        // read minify:
        //   the ", " or "," is read out, this case is unlikely
        // guess it occurs when the document is using some `CHAR_TYPE_SPACE` characters
        // other than space itself as indent, like, tabs.
        SKIP_CONSECUTIVE_SPACES(cur);
        goto arr_val_begin;
    }
    if ((*cur == 'i' || *cur == 'I' || *cur == 'N')) {
        PyObject *number_obj = read_inf_or_nan(false, &cur, end);
        if (likely(number_obj && pyyjson_push_obj(decode_obj_stack_info, number_obj))) {
            incr_decode_ctn_size(decode_ctn_info->ctn);
            goto arr_val_end;
        }
        goto fail_character_val;
    }

    goto fail_character_val;

arr_val_end:;
#if DECODE_READ_PRETTY
    // ",\n"
    if (cmpeq_2chars(cur, _CommaReturn, end)) {
#else
    // ", "
    if (cmpeq_2chars(cur, _CommaSpace, end)) {
#endif
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
    if (WRAPPED_CHAR_IS_SPACE(cur)) {
        // unlikely case, we expect a "," or "]" but not found right after the value
        cur++;
        if (*cur == ' ') fast_skip_spaces(&cur, end);
        if (WRAPPED_CHAR_IS_SPACE(cur)) {
            SKIP_CONSECUTIVE_SPACES(cur);
        }
        //
        goto arr_val_end;
    }

    goto fail_character_arr_end;

arr_end:
    assert(decode_ctn_is_arr(decode_ctn_info->ctn));
    if (!pyyjson_decode_arr(decode_obj_stack_info, get_decode_ctn_len(decode_ctn_info->ctn))) goto failed_cleanup;
    /* pop parent as current container */
    if (unlikely(decode_ctn_info->ctn-- == decode_ctn_info->ctn_start)) {
        goto doc_end;
    }

    incr_decode_ctn_size(decode_ctn_info->ctn);
    if (DECODE_READ_PRETTY && *cur == '\n') cur++;
    if (!decode_ctn_is_arr(decode_ctn_info->ctn)) {
        goto obj_val_end;
    } else {
        goto arr_val_end;
    }

obj_begin:
    /* push container */
    if (unlikely(!ctn_grow_check(decode_ctn_info))) goto fail_ctn_grow;
    set_decode_ctn(decode_ctn_info->ctn, 0, false);
    if (DECODE_READ_PRETTY && *cur == '\n') cur++;

obj_key_begin:
#if DECODE_READ_PRETTY
    if (*cur == ' ') {
        // cur++;
        // if (*cur == ' ')
        fast_skip_spaces(&cur, end);
    }
    // #if PYYJSON_IS_REAL_GCC
    //     while (true) REPEAT_CALL_16({
    //         if (byte_match_2((void *)cur, "  ")) cur += 2;
    //         else
    //             break;
    //     })
    // #else
    //     while (true) REPEAT_CALL_16({
    //         if (likely(byte_match_2(cur, "  "))) cur += 2;
    //         else
    //             break;
    //     })
    // #endif
#endif

    if (likely(*cur == '"')) {
        cur++;
        process_str_jump_flag = 0; // key
        goto process_str;
    }
    if (likely(*cur == '}')) {
        cur++;
        if (likely(get_decode_ctn_len(decode_ctn_info->ctn) == 0)) goto obj_end;
        goto fail_trailing_comma;
    }
    if (WRAPPED_CHAR_IS_SPACE(cur)) {
        // for both read pretty and minify:
        //   likely occurs when the document is using some `CHAR_TYPE_SPACE` characters
        //   other than space as indent.
        //   see the comment in `arr_val_begin` for more details.
        SKIP_CONSECUTIVE_SPACES(cur);
        goto obj_key_begin;
    }
    goto fail_character_obj_key;

obj_key_end:;
    // #if DECODE_READ_PRETTY
    // ": "
    if (cmpeq_2chars(cur, _ColonSpace, end)) {
        cur += 2;
        goto obj_val_begin;
    }
    // #endif
    if (*cur == ':') {
        cur++;
        goto obj_val_begin;
    }
    if (WRAPPED_CHAR_IS_SPACE(cur)) {
        // unlikely case, we expect a colon here
        cur++;
        if (*cur == ' ') fast_skip_spaces(&cur, end);
        if (WRAPPED_CHAR_IS_SPACE(cur)) {
            SKIP_CONSECUTIVE_SPACES(cur);
        }
        //
        goto obj_key_end;
    }
    goto fail_character_obj_sep;

obj_val_begin:
    if (*cur == '"') {
        cur++;
        process_str_jump_flag = 1; // obj value
        goto process_str;
    }
    if (char_is_number(*cur)) {
        PyObject *number_obj = READ_NUMBER(&cur, end);
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
        if (likely(_read_true(&cur, end) && pyyjson_decode_true(decode_obj_stack_info))) {
            incr_decode_ctn_size(decode_ctn_info->ctn);
            goto obj_val_end;
        }
        goto fail_literal_true;
    }
    if (*cur == 'f') {
        if (likely(_read_false(&cur, end) && pyyjson_decode_false(decode_obj_stack_info))) {
            incr_decode_ctn_size(decode_ctn_info->ctn);
            goto obj_val_end;
        }
        goto fail_literal_false;
    }
    if (*cur == 'n') {
        if (likely(_read_null(&cur, end) && pyyjson_decode_null(decode_obj_stack_info))) {
            incr_decode_ctn_size(decode_ctn_info->ctn);
            goto obj_val_end;
        }
        if (likely(_read_nan(&cur, end) && pyyjson_decode_nan(decode_obj_stack_info, false))) {
            incr_decode_ctn_size(decode_ctn_info->ctn);
            goto obj_val_end;
        }
        goto fail_literal_null;
    }
    if (WRAPPED_CHAR_IS_SPACE(cur)) {
        // read pretty:
        //   the ": " is read out, this character is likely to be "\n", then we should skip spaces before new line
        // read minify:
        //   the ": " or ":" is read out, this is an unlikely case
        cur++;
#if DECODE_READ_PRETTY
        if (*cur == ' ') fast_skip_spaces(&cur, end);
#endif
        if (WRAPPED_CHAR_IS_SPACE(cur)) {
            // handle unlikely cases
            SKIP_CONSECUTIVE_SPACES(cur);
        }
        goto obj_val_begin;
    }
    if ((*cur == 'i' || *cur == 'I' || *cur == 'N')) {
        PyObject *number_obj = read_inf_or_nan(false, &cur, end);
        if (likely(number_obj && pyyjson_push_obj(decode_obj_stack_info, number_obj))) {
            incr_decode_ctn_size(decode_ctn_info->ctn);
            goto obj_val_end;
        }
        goto fail_character_val;
    }

    goto fail_character_val;

obj_val_end:;
#if DECODE_READ_PRETTY
    // ",\n"
    if (cmpeq_2chars(cur, _CommaReturn, end)) {
#else
    // ", "
    if (cmpeq_2chars(cur, _CommaSpace, end)) {
#endif
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
    if (WRAPPED_CHAR_IS_SPACE(cur)) {
        // unlikely case
        cur++;
        if (*cur == ' ') fast_skip_spaces(&cur, end);
        if (WRAPPED_CHAR_IS_SPACE(cur)) {
            SKIP_CONSECUTIVE_SPACES(cur);
        }
        //
        goto obj_val_end;
    }

    goto fail_character_obj_end;

obj_end:
    assert(!decode_ctn_is_arr(decode_ctn_info->ctn));
    if (unlikely(!pyyjson_decode_obj(decode_obj_stack_info, get_decode_ctn_len(decode_ctn_info->ctn)))) goto failed_cleanup;
    /* pop container */
    /* point to the next value */
    if (unlikely(decode_ctn_info->ctn-- == decode_ctn_info->ctn_start)) {
        goto doc_end;
    }
    incr_decode_ctn_size(decode_ctn_info->ctn);
    if (DECODE_READ_PRETTY && *cur == '\n') cur++;
    if (decode_ctn_is_arr(decode_ctn_info->ctn)) {
        goto arr_val_end;
    } else {
        goto obj_val_end;
    }

doc_end:
    /* check invalid contents after json document */
    if (unlikely(cur < end)) {
        if (*cur == ' ') fast_skip_spaces(&cur, end);
        if (WRAPPED_CHAR_IS_SPACE(cur)) {
            SKIP_CONSECUTIVE_SPACES(cur);
        }
        if (unlikely(cur < end)) goto fail_garbage;
    }

success:;
    PyObject *obj = *decode_obj_stack_info->result_stack;
    assert(decode_ctn_info->ctn == decode_ctn_info->ctn_start - 1);
    assert(decode_obj_stack_info->cur_write_result_addr == decode_obj_stack_info->result_stack + 1);
    assert(obj && !PyErr_Occurred());
    assert(obj->ob_refcnt == 1);
    // free string buffer
    if (need_dealloc) {
        free((void *)((u8 *)string_buffer_head - TAIL_PADDING));
    }
    // free obj stack buffer if allocated dynamically
    if (unlikely(decode_obj_stack_info->result_stack_end - decode_obj_stack_info->result_stack > PYYJSON_DECODE_OBJ_BUFFER_INIT_SIZE)) {
        free(decode_obj_stack_info->result_stack);
    }

    return obj;

#define return_err(_pos, _type, _msg)                                                                     \
    do {                                                                                                  \
        if (_type == JSONDecodeError) {                                                                   \
            PyErr_Format(JSONDecodeError, "%s, at position %zu", _msg, ((_src_t *)_pos) - (_src_t *)dat); \
        } else {                                                                                          \
            PyErr_SetString(_type, _msg);                                                                 \
        }                                                                                                 \
        goto failed_cleanup;                                                                              \
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
    if (need_dealloc) {
        free((void *)((u8 *)string_buffer_head - TAIL_PADDING));
    }
    // free obj stack buffer if allocated dynamically
    if (unlikely(decode_obj_stack_info->result_stack_end - decode_obj_stack_info->result_stack > PYYJSON_DECODE_OBJ_BUFFER_INIT_SIZE)) {
        free(decode_obj_stack_info->result_stack);
    }
    return NULL;
#undef return_err
}

#undef SKIP_CONSECUTIVE_SPACES
#undef WRAPPED_CHAR_IS_SPACE
