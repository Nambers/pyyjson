
#include "test_common.h"

#define COUNT_OF(x) ((sizeof(x) / sizeof(0 [x])) / ((size_t)(!(sizeof(x) % sizeof(0 [x])))))

typedef struct TestArgSettings {
    int valid;
    PyObject *args[16];
} TestArgSettings;

typedef struct TestResult {
    PyObject *decoded;
    PyObject *encoded;
} TestResult;

PyObject *pyyjson_module = NULL;
PyObject *decode_func = NULL;
PyObject *encode_func = NULL;

TestArgSettings decode_arg_settings[16];
TestArgSettings encode_arg_settings[16];

static bool init_test_arg_settings(void) {
    PyObject *pylong_1 = NULL, *pylong_2 = NULL;
    memset(decode_arg_settings, 0, sizeof(decode_arg_settings));
    memset(encode_arg_settings, 0, sizeof(encode_arg_settings));
    pylong_1 = PyLong_FromLong(1);
    pylong_2 = PyLong_FromLong(2);
    if (!pylong_1 || !pylong_2) goto fail;
    //
    decode_arg_settings[0].valid = 1;
    //
    encode_arg_settings[0].valid = 1;
    encode_arg_settings[1].valid = 1;
    encode_arg_settings[1].args[1] = pylong_1;
    encode_arg_settings[2].valid = 1;
    encode_arg_settings[2].args[1] = pylong_2;
    return true;
fail:;
    Py_XDECREF(pylong_1);
    return false;
}

int LLVMFuzzerInitialize(int *argc, char ***argv) {
    initialize_cpython();
    pyyjson_module = import_pyyjson();
    if (!pyyjson_module) goto fail;
    decode_func = PyObject_GetAttrString(pyyjson_module, "loads");
    if (!decode_func) goto fail;
    encode_func = PyObject_GetAttrString(pyyjson_module, "dumps");
    if (!encode_func) goto fail;
    if (!init_test_arg_settings()) goto fail;
    return 0;
fail:;
    Py_XDECREF(pyyjson_module);
    Py_XDECREF(decode_func);
    Py_XDECREF(encode_func);
    __builtin_trap();
    return -1;
}

static void parse_input_to_bytes(const u8 *data, usize size, PyObject **bytes, PyObject **str) {
    *bytes = PyBytes_FromStringAndSize((const char *)data, (Py_ssize_t)size);
    if (!*bytes) PyErr_Clear();
    *str = PyUnicode_FromStringAndSize((const char *)data, (Py_ssize_t)size);
    if (!*str) PyErr_Clear();
}

force_inline usize get_vectorcall_length(TestArgSettings *setting) {
    usize vectorcall_length = 1;
    for (usize k = 1; k < COUNT_OF(setting->args); k++) {
        if (!setting->args[k]) break;
        vectorcall_length++;
    }
    return vectorcall_length;
}

void test_one_decode_encode(PyObject *input, TestArgSettings *decode_setting, TestArgSettings *encode_setting, TestResult *result) {
    PyObject *decoded = NULL, *encoded = NULL;
    usize vectorcall_length;
    //
    decode_setting->args[0] = input;
    vectorcall_length = get_vectorcall_length(decode_setting);
    decoded = PyObject_Vectorcall(decode_func, decode_setting->args, vectorcall_length, NULL);
    if (!decoded) {
        goto call_fail;
    }
    //
    encode_setting->args[0] = decoded;
    vectorcall_length = get_vectorcall_length(encode_setting);
    encoded = PyObject_Vectorcall(encode_func, encode_setting->args, vectorcall_length, NULL);
    if (!encoded) {
        goto call_fail;
    }
    goto done;
call_fail:;
    PyErr_Clear();
    goto done;
done:;
    decode_setting->args[0] = NULL;
    encode_setting->args[0] = NULL;
    result->decoded = decoded;
    result->encoded = encoded;
}

int LLVMFuzzerTestOneInput(const u8 *data, usize size) {
    if (!size) return 0;
    PyObject *bytes, *str;
    // PyObject *decoded_from_bytes = NULL, *decoded_from_str = NULL;
    // PyObject *encoded_from_bytes_source = NULL, *encoded_from_str_source = NULL;
    // parse input to bytes and str
    parse_input_to_bytes(data, size, &bytes, &str);
    if (!bytes && !str) {
        // invalid input
        return -1;
    }
    for (usize i = 0; i < COUNT_OF(decode_arg_settings); i++) {
        if (!decode_arg_settings[i].valid) break;
        for (usize j = 0; j < COUNT_OF(encode_arg_settings); j++) {
            if (!encode_arg_settings[j].valid) break;
            TestResult bytes_result;
            TestResult str_result;
            if (bytes) {
                test_one_decode_encode(bytes, &decode_arg_settings[i], &encode_arg_settings[j], &bytes_result);
            }
            if (str) {
                test_one_decode_encode(str, &decode_arg_settings[i], &encode_arg_settings[j], &str_result);
            }
            // compare. TODO impl this
            //
            // cleanup
            Py_XDECREF(bytes_result.decoded);
            Py_XDECREF(bytes_result.encoded);
            Py_XDECREF(str_result.decoded);
            Py_XDECREF(str_result.encoded);
        }
    }
    Py_XDECREF(bytes);
    Py_XDECREF(str);
    return 0;
}
