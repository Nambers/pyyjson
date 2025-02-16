/*
 * Required macros:
 *   READ_ROOT_IMPL, points to the function name
 *   DECODE_READ_PRETTY, true/false
 */


/** Read JSON document (accept all style, but optimized for pretty). */
static force_noinline PyObject *READ_ROOT_IMPL(const _FROM_TYPE *dat, Py_ssize_t len) {
    static _FROM_TYPE _dotReturn[2] = {',', '\n'};

    const _FROM_TYPE *cur = dat;
    const _FROM_TYPE *const end = cur + len;
    // container stack info
    DecodeCtnStackInfo _decode_ctn_info;
    DecodeCtnStackInfo *decode_ctn_info = &_decode_ctn_info;
    // object stack info
    DecodeObjStackInfo _decode_obj_stack_info;
    DecodeObjStackInfo *const decode_obj_stack_info = &_decode_obj_stack_info;
    memset(decode_ctn_info, 0, sizeof(DecodeCtnStackInfo));
    memset(decode_obj_stack_info, 0, sizeof(DecodeObjStackInfo));
    // init
    if (!init_decode_ctn_stack_info(decode_ctn_info) || !init_decode_obj_stack_info(decode_obj_stack_info)) goto failed_cleanup;
    _FROM_TYPE *string_buffer_head;
    bool need_dealloc = false;
    if (unlikely(!CHECK_AND_RESERVE_STR_BUFFER(len, &string_buffer_head, &need_dealloc))) {
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

arr_begin:
    /* save current container */
    /* create a new array value, save parent container offset */
    if (unlikely(!ctn_grow_check(decode_ctn_info))) goto fail_ctn_grow;
    set_decode_ctn(decode_ctn_info->ctn, 0, true);

    /* push the new array value as current container */
    if (DECODE_READ_PRETTY && *cur == '\n') cur++;

arr_val_begin:
#if DECODE_READ_PRETTY
    if (cur < end && cur[0] == ' ') {
        cur++;
        if (*cur == ' ')
            FAST_SKIP_SPACES(&cur, end);
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
        PyObject *str_obj = READ_STR(&cur, end, string_buffer_head, false);
        if (likely(str_obj && pyyjson_push_obj(decode_obj_stack_info, str_obj))) {
            incr_decode_ctn_size(decode_ctn_info->ctn);
            goto arr_val_end;
        }
        goto fail_string;
    }
    if (*cur == 't') {
        if (likely(_READ_TRUE(&cur, end) && pyyjson_decode_true(decode_obj_stack_info))) {
            incr_decode_ctn_size(decode_ctn_info->ctn);
            goto arr_val_end;
        }
        goto fail_literal_true;
    }
    if (*cur == 'f') {
        if (likely(_READ_FALSE(&cur, end) && pyyjson_decode_false(decode_obj_stack_info))) {
            incr_decode_ctn_size(decode_ctn_info->ctn);
            goto arr_val_end;
        }
        goto fail_literal_false;
    }
    if (*cur == 'n') {
        if (likely(_READ_NULL(&cur, end) && pyyjson_decode_null(decode_obj_stack_info))) {
            incr_decode_ctn_size(decode_ctn_info->ctn);
            goto arr_val_end;
        }
        if (likely(_READ_NAN(&cur, end) && pyyjson_decode_nan(decode_obj_stack_info, false))) {
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
        FAST_SKIP_SPACES(&cur, end);
        if (char_is_space(*cur)) cur++;
        // while (char_is_space(*++cur));
        goto arr_val_begin;
    }
    if ((*cur == 'i' || *cur == 'I' || *cur == 'N')) {
        PyObject *number_obj = READ_INF_OR_NAN(false, &cur, end);
        if (likely(number_obj && pyyjson_push_obj(decode_obj_stack_info, number_obj))) {
            incr_decode_ctn_size(decode_ctn_info->ctn);
            goto arr_val_end;
        }
        goto fail_character_val;
    }

    goto fail_character_val;

arr_val_end:;
#if DECODE_READ_PRETTY
    if (CMP_2_CHARS_EQ(cur, _dotReturn, end)) {
        cur += 2;
        goto arr_val_begin;
    }
#endif
    if (*cur == ',') {
        cur++;
        goto arr_val_begin;
    }
    if (*cur == ']') {
        cur++;
        goto arr_end;
    }
    if (char_is_space(*cur)) {
        cur++;
        if (*cur == ' ') FAST_SKIP_SPACES(&cur, end);
        if (char_is_space(*cur)) cur++;
        // while (char_is_space(*++cur));
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
    if (cur < end && cur[0] == ' ') {
        cur++;
        if (*cur == ' ')
            FAST_SKIP_SPACES(&cur, end);
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
        PyObject *str_obj = READ_STR(&cur, end, string_buffer_head, true);
        ;
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
        FAST_SKIP_SPACES(&cur, end);
        if (char_is_space(*cur)) cur++;
        // while (char_is_space(*++cur));
        goto obj_key_begin;
    }
    goto fail_character_obj_key;

obj_key_end:;
#if DECODE_READ_PRETTY
    {
        static _FROM_TYPE _t[2] = {':', ' '};
        if (CMP_2_CHARS_EQ(cur, _t, end)) {
            cur += 2;
            goto obj_val_begin;
        }
    }
#endif
    if (*cur == ':') {
        cur++;
        goto obj_val_begin;
    }
    if (char_is_space(*cur)) {
        FAST_SKIP_SPACES(&cur, end);
        if (char_is_space(*cur)) cur++;
        // while (char_is_space(*++cur));
        goto obj_key_end;
    }
    goto fail_character_obj_sep;

obj_val_begin:
    if (*cur == '"') {
        cur++;
        PyObject *str_obj = READ_STR(&cur, end, string_buffer_head, false);
        ;
        if (likely(str_obj && pyyjson_push_obj(decode_obj_stack_info, str_obj))) {
            incr_decode_ctn_size(decode_ctn_info->ctn);
            goto obj_val_end;
        }
        goto fail_string;
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
        if (likely(_READ_TRUE(&cur, end) && pyyjson_decode_true(decode_obj_stack_info))) {
            incr_decode_ctn_size(decode_ctn_info->ctn);
            goto obj_val_end;
        }
        goto fail_literal_true;
    }
    if (*cur == 'f') {
        if (likely(_READ_FALSE(&cur, end) && pyyjson_decode_false(decode_obj_stack_info))) {
            incr_decode_ctn_size(decode_ctn_info->ctn);
            goto obj_val_end;
        }
        goto fail_literal_false;
    }
    if (*cur == 'n') {
        if (likely(_READ_NULL(&cur, end) && pyyjson_decode_null(decode_obj_stack_info))) {
            incr_decode_ctn_size(decode_ctn_info->ctn);
            goto obj_val_end;
        }
        if (likely(_READ_NAN(&cur, end) && pyyjson_decode_nan(decode_obj_stack_info, false))) {
            incr_decode_ctn_size(decode_ctn_info->ctn);
            goto obj_val_end;
        }
        goto fail_literal_null;
    }
    if (char_is_space(*cur)) {
        FAST_SKIP_SPACES(&cur, end);
        if (char_is_space(*cur)) cur++;
        // while (char_is_space(*++cur));
        goto obj_val_begin;
    }
    if ((*cur == 'i' || *cur == 'I' || *cur == 'N')) {
        PyObject *number_obj = READ_INF_OR_NAN(false, &cur, end);
        if (likely(number_obj && pyyjson_push_obj(decode_obj_stack_info, number_obj))) {
            incr_decode_ctn_size(decode_ctn_info->ctn);
            goto obj_val_end;
        }
        goto fail_character_val;
    }

    goto fail_character_val;

obj_val_end:;
#if DECODE_READ_PRETTY
    if (CMP_2_CHARS_EQ(cur, _dotReturn, end)) {
        cur += 2;
        goto obj_key_begin;
    }
#endif
    if (likely(*cur == ',')) {
        cur++;
        goto obj_key_begin;
    }
    if (likely(*cur == '}')) {
        cur++;
        goto obj_end;
    }
    if (char_is_space(*cur)) {
        FAST_SKIP_SPACES(&cur, end);
        if (char_is_space(*cur)) cur++;
        // while (char_is_space(*++cur));
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
        FAST_SKIP_SPACES(&cur, end);
        if (char_is_space(*cur)) cur++;
        // while (char_is_space(*cur)) cur++;
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

#define return_err(_pos, _type, _msg)                                                                             \
    do {                                                                                                          \
        if (_type == JSONDecodeError) {                                                                           \
            PyErr_Format(JSONDecodeError, "%s, at position %zu", _msg, ((_FROM_TYPE *)_pos) - (_FROM_TYPE *)dat); \
        } else {                                                                                                  \
            PyErr_SetString(_type, _msg);                                                                         \
        }                                                                                                         \
        goto failed_cleanup;                                                                                      \
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
