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
int new_write_as = PYYJSON_MAX(_read_state.max_char_type, COMPILE_READ_UCS_LEVEL);
bool need_check_max_char = _read_state.max_char_type < COMPILE_UCS_LEVEL;
int jump_code = new_write_as + (need_check_max_char ? 4 : 0);
if (_read_state.need_copy) {
    switch (jump_code) {
#if COMPILE_UCS_LEVEL <= 1
        case 1: {
            goto loop_1_t_f;
        }
#endif
#if COMPILE_UCS_LEVEL <= 2
        case 2: {
            goto loop_2_t_f;
        }
#endif
        case 4: {
            goto loop_4_t_f;
        }
#if COMPILE_UCS_LEVEL == 1
        case 5: {
            goto loop_1_t_t;
        }
#endif
#if COMPILE_UCS_LEVEL == 2
        case 6: {
            goto loop_2_t_t;
        }
#endif
#if COMPILE_UCS_LEVEL == 4
        case 8: {
            goto loop_4_t_t;
        }
#endif
        default: {
            assert(false);
            Py_UNREACHABLE();
        }
    }
} else {
    switch (jump_code) {
#if COMPILE_UCS_LEVEL <= 1
        case 1: {
            goto loop_1_f_f;
        }
#endif
#if COMPILE_UCS_LEVEL == 2
        case 2: {
            goto loop_2_f_f;
        }
#endif
#if COMPILE_UCS_LEVEL == 4
        case 4: {
            goto loop_4_f_f;
        }
#endif
#if COMPILE_UCS_LEVEL == 1
        case 5: {
            goto loop_1_f_t;
        }
#endif
#if COMPILE_UCS_LEVEL == 2
        case 6: {
            goto loop_2_f_t;
        }
#endif
#if COMPILE_UCS_LEVEL == 4
        case 8: {
            goto loop_4_f_t;
        }
#endif
        default: {
            assert(false);
            Py_UNREACHABLE();
        }
    }
}
