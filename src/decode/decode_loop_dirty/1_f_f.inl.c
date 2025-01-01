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

// knowledge before loop: max_char_type == COMPILE_UCS_LEVEL && max_char_type <= 1
assert(_read_state.need_copy);
switch (_read_state.max_char_type) {
#if COMPILE_UCS_LEVEL == PYYJSON_STRING_TYPE_ASCII
    case PYYJSON_STRING_TYPE_ASCII: {
        // max char can only be greater than before
        // so before this loop we have
        // max_char_type == COMPILE_UCS_LEVEL <= 0
        // therefore, need_check_max_char is false
        // 1_t_f
        goto loop_1_t_f;
    }
#endif
    case PYYJSON_STRING_TYPE_LATIN1: {
        // max char can only be greater than before.
        // so "new max_char_type" == PYYJSON_STRING_TYPE_LATIN1 >= "old max_char_type" == COMPILE_UCS_LEVEL
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
