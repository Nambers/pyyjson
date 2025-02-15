#include "decode.h"
#include <threads.h>

bool _pyyjson_decode_obj_stack_resize(DecodeObjStackInfo *restrict decode_obj_stack_info) {
    // resize
    if (likely(PYYJSON_DECODE_OBJ_BUFFER_INIT_SIZE == decode_obj_stack_info->result_stack_end - decode_obj_stack_info->result_stack)) {
        void *new_buffer = malloc(sizeof(PyObject *) * (PYYJSON_DECODE_OBJ_BUFFER_INIT_SIZE << 1));
        if (unlikely(!new_buffer)) {
            PyErr_NoMemory();
            return false;
        }
        memcpy(new_buffer, decode_obj_stack_info->result_stack, sizeof(PyObject *) * PYYJSON_DECODE_OBJ_BUFFER_INIT_SIZE);
        decode_obj_stack_info->result_stack = (PyObject **)new_buffer;
        decode_obj_stack_info->cur_write_result_addr = decode_obj_stack_info->result_stack + PYYJSON_DECODE_OBJ_BUFFER_INIT_SIZE;
        decode_obj_stack_info->result_stack_end = decode_obj_stack_info->result_stack + (PYYJSON_DECODE_OBJ_BUFFER_INIT_SIZE << 1);
    } else {
        Py_ssize_t old_capacity = decode_obj_stack_info->result_stack_end - decode_obj_stack_info->result_stack;
        if (unlikely((PY_SSIZE_T_MAX >> 1) < old_capacity)) {
            PyErr_NoMemory();
            return false;
        }
        Py_ssize_t new_capacity = old_capacity << 1;
        void *new_buffer = realloc(decode_obj_stack_info->result_stack, sizeof(PyObject *) * new_capacity);
        if (unlikely(!new_buffer)) {
            PyErr_NoMemory();
            return false;
        }
        decode_obj_stack_info->result_stack = (PyObject **)new_buffer;
        decode_obj_stack_info->cur_write_result_addr = decode_obj_stack_info->result_stack + old_capacity;
        decode_obj_stack_info->result_stack_end = decode_obj_stack_info->result_stack + new_capacity;
    }
    return true;
}

thread_local u8 pyyjson_string_buffer[PYYJSON_STRING_BUFFER_SIZE];
pyyjson_cache_type AssociativeKeyCache[PYYJSON_KEY_CACHE_SIZE];
