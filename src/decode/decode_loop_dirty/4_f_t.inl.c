/*
 * NOTE:
 * write_as = max(_read_state.max_char_type, COMPILE_READ_UCS_LEVEL)
 * need_copy == <escape is met>
 * need_check_max_char = max_char_type < COMPILE_UCS_LEVEL
 * 
 * additional note:
 *  1. need_copy == false => max_char_type <= COMPILE_UCS_LEVEL
 *      !need_copy && !need_check_max_char => max_char_type == COMPILE_UCS_LEVEL
 *      COMPILE_UCS_LEVEL < max_char_type == write_as => need_copy == true
 *  2. COMPILE_READ_UCS_LEVEL = COMPILE_UCS_LEVEL ? COMPILE_UCS_LEVEL : 1
 *  3. max_char_type == 4 || COMPILE_UCS_LEVEL == 0 => need_check_max_char == false
 */

// knowledge before loop: max_char_type < COMPILE_UCS_LEVEL == 4
if (_read_state.need_copy) {
    // escaped.
    if (_read_state.max_char_type == PYYJSON_STRING_TYPE_UCS4) {
        goto loop_4_t_f;
    } else {
        assert(_read_state.max_char_type >= PYYJSON_STRING_TYPE_ASCII && _read_state.max_char_type <= PYYJSON_STRING_TYPE_UCS2);
        goto loop_4_t_t;
    }
} else {
    // check max char is dirty without any escape, max_char_type must have been updated
#if !defined(NDEBUG)
    if (_read_state.max_char_type < COMPILE_UCS_LEVEL) {
        assert(false);
        // loop_4_f_t (self), not really dirty.
        goto loop_4_f_t;
    }
#endif
    assert(_read_state.max_char_type == COMPILE_UCS_LEVEL);
    // 4_f_f
    goto loop_4_f_f;
}
