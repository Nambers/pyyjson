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

// knowledge before loop: max_char_type == 0 && COMPILE_UCS_LEVEL == 1
if (_read_state.need_copy) {
    // escaped.
    switch (_read_state.max_char_type) {
        case PYYJSON_STRING_TYPE_ASCII: {
            // max_char_type not updated
            // 1_t_t
            goto loop_1_t_t;
        }
        case PYYJSON_STRING_TYPE_LATIN1: {
            // 1_t_f
            goto loop_1_t_f;
        }
        case PYYJSON_STRING_TYPE_UCS2: {
            // 2_t_f
            goto loop_2_t_f;
        }
        case PYYJSON_STRING_TYPE_UCS4: {
            // 4_t_f
            goto loop_4_t_f;
        }
        default: {
            assert(false);
            Py_UNREACHABLE();
        }
    }
} else {
    // "check max char" is dirty without any escape, max_char_type must have been updated
    // so we must have "old max_char_type" == 0 and
    // 1 == "new max_char_type" <= COMPILE_UCS_LEVEL == 1,
    // since need_copy is false
    assert(_read_state.max_char_type == COMPILE_UCS_LEVEL && COMPILE_UCS_LEVEL == 1);
    goto loop_1_f_f;
}
