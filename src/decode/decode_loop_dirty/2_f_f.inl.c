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

// knowledge before loop: 2 == max_char_type == COMPILE_UCS_LEVEL
assert(_read_state.need_copy);
switch (_read_state.max_char_type) {
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
