#define PY_SSIZE_T_CLEAN
#include "pyyjson.h"
#include "tls.h"

#if PYYJSON_X86
#    if defined(_MSC_VER)
#        define cpuid_count(info, x) __cpuidex(info, x, 0)
#        define cpuid(info, x) __cpuid(info, x)
#    else
#        include <cpuid.h>

force_inline void cpuid_count(int *info, int x) {
    __cpuid_count(x, 0, info[0], info[1], info[2], info[3]);
}

force_inline void cpuid(int *info, int x) {
    __cpuid(x, info[0], info[1], info[2], info[3]);
}
#    endif
#endif

typedef PyObject *pyyjson_cache_type;

extern pyyjson_cache_type AssociativeKeyCache[PYYJSON_KEY_CACHE_SIZE];

PyObject *pyyjson_Encode(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *pyyjson_Decode(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *pyyjson_FileEncode(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *pyyjson_DecodeFile(PyObject *self, PyObject *args, PyObject *kwargs);
#if PYYJSON_BUILD_BENCHMARK
PyObject *run_unicode_accumulate_benchmark(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *run_object_accumulate_benchmark(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *run_object_benchmark(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *inspect_pyunicode(PyObject *self, PyObject *args, PyObject *kwargs);
#endif
PyObject *pyyjson_print_current_features(PyObject *self, PyObject *);
PyObject *pyyjson_get_current_features(PyObject *self, PyObject *);

PyObject *JSONDecodeError = NULL;
PyObject *JSONEncodeError = NULL;

static PyMethodDef pyyjson_Methods[] = {
        {"encode", (PyCFunction)pyyjson_Encode, METH_VARARGS | METH_KEYWORDS, "dumps(obj, indent=None)\n--\n\nConverts arbitrary object recursively into JSON."},
        {"decode", (PyCFunction)pyyjson_Decode, METH_VARARGS | METH_KEYWORDS, "decode(s)\n--\n\nConverts JSON as string to dict object structure."},
        {"dumps", (PyCFunction)pyyjson_Encode, METH_VARARGS | METH_KEYWORDS, "dumps(obj, indent=None)\n--\n\nConverts arbitrary object recursively into JSON."},
        {"loads", (PyCFunction)pyyjson_Decode, METH_VARARGS | METH_KEYWORDS, "loads(s)\n--\n\nConverts JSON as string to dict object structure."},
        {"print_current_features", pyyjson_print_current_features, METH_NOARGS, "print_current_features()\n--\n\nPrints current features."},
        {"get_current_features", pyyjson_get_current_features, METH_NOARGS, "get_current_features()\n--\n\nGet current features."},
#if PYYJSON_BUILD_BENCHMARK
        {"run_unicode_accumulate_benchmark", (PyCFunction)run_unicode_accumulate_benchmark, METH_VARARGS | METH_KEYWORDS, "Benchmark."},
        {"run_object_accumulate_benchmark", (PyCFunction)run_object_accumulate_benchmark, METH_VARARGS | METH_KEYWORDS, "Benchmark."},
        {"run_object_benchmark", (PyCFunction)run_object_benchmark, METH_VARARGS | METH_KEYWORDS, "Benchmark."},
        {"inspect_pyunicode", (PyCFunction)inspect_pyunicode, METH_VARARGS | METH_KEYWORDS, "Inspect PyUnicode."},
#endif
        {NULL, NULL, 0, NULL} /* Sentinel */
};

static void module_free(void *m);

static struct PyModuleDef moduledef = {
        PyModuleDef_HEAD_INIT,
        "pyyjson",
        0,               /* m_doc */
        0,               /* m_size */
        pyyjson_Methods, /* m_methods */
        NULL,            /* m_slots */
        NULL,            /* m_traverse */
        NULL,            /* m_clear */
        module_free      /* m_free */
};

static void module_free(void *m) {
    for (size_t i = 0; i < PYYJSON_KEY_CACHE_SIZE; i++) {
        Py_XDECREF(AssociativeKeyCache[i]);
    }

    if (unlikely(!pyyjson_tls_free())) {
        // critical
        printf("pyyjson: failed to free TLS\n");
    }
#if PYYJSON_ENABLE_TRACE
    size_t cached = 0;
    for (size_t i = 0; i < PYYJSON_KEY_CACHE_SIZE; i++) {
        if (AssociativeKeyCache[i]) cached++;
    }
    printf("key cache: %zu/%d\n", cached, PYYJSON_KEY_CACHE_SIZE);
#endif // PYYJSON_ENABLE_TRACE
}

#if PY_MINOR_VERSION >= 13
PyTypeObject *PyNone_Type = NULL;

void _init_PyNone_Type(PyTypeObject *none_type) {
    PyNone_Type = none_type;
}
#endif


PyMODINIT_FUNC PyInit_pyyjson(void) {
    PyObject *module;

    // This function is not supported in PyPy.
    if ((module = PyState_FindModule(&moduledef)) != NULL) {
        Py_INCREF(module);
        return module;
    }

    module = PyModule_Create(&moduledef);
    if (module == NULL) {
        return NULL;
    }

    PyModule_AddStringConstant(module, "__version__", PYYJSON_VERSION_STRING);

    JSONDecodeError = PyErr_NewException("pyyjson.JSONDecodeError", PyExc_ValueError, NULL);
    Py_XINCREF(JSONDecodeError);
    if (PyModule_AddObject(module, "JSONDecodeError", JSONDecodeError) < 0) {
        Py_XDECREF(JSONDecodeError);
        Py_CLEAR(JSONDecodeError);
        Py_DECREF(module);
        return NULL;
    }

    JSONEncodeError = PyErr_NewException("pyyjson.JSONEncodeError", PyExc_ValueError, NULL);
    Py_XINCREF(JSONEncodeError);
    if (PyModule_AddObject(module, "JSONEncodeError", JSONEncodeError) < 0) {
        Py_XDECREF(JSONEncodeError);
        Py_CLEAR(JSONEncodeError);
        Py_DECREF(module);
        return NULL;
    }

    // TLS init.
    if (unlikely(!pyyjson_tls_init())) {
        PyErr_SetString(PyExc_RuntimeError, "Failed to initialize TLS");
        Py_XDECREF(JSONEncodeError);
        Py_CLEAR(JSONEncodeError);
        Py_DECREF(module);
        return NULL;
    }

    // do pyyjson internal init.
    memset(AssociativeKeyCache, 0, sizeof(AssociativeKeyCache));

#if PY_MINOR_VERSION >= 13
    _init_PyNone_Type(Py_TYPE(Py_None));
#endif

    return module;
}

#if BUILD_MULTI_LIB

#    if PYYJSON_X86
typedef enum X86SIMDFeatureLevel {
    X86SIMDFeatureLevelSSE2 = 0,
    X86SIMDFeatureLevelSSE4_2 = 1,
    X86SIMDFeatureLevelAVX2 = 2,
    X86SIMDFeatureLevelAVX512 = 3,
    X86SIMDFeatureLevelMAX = 4,
} X86SIMDFeatureLevel;

#        define PLATFORM_SIMD_LEVEL X86SIMDFeatureLevel
#    elif PYYJSON_AARCH
typedef enum AArchSIMDFeatureLevel {
    AArchSIMDFeatureLevelNEON = 0,
} AArchSIMDFeatureLevel;

#        define PLATFORM_SIMD_LEVEL AArchSIMDFeatureLevel
#    endif

PyObject *pyyjson_Encode_avx512(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *pyyjson_Encode_avx2(PyObject *self, PyObject *args, PyObject *kwargs);
// PyObject *pyyjson_Encode_sse4_2(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *pyyjson_Encode_sse2(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *pyyjson_Decode_avx512(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *pyyjson_Decode_avx2(PyObject *self, PyObject *args, PyObject *kwargs);
// PyObject *pyyjson_Decode_sse4_2(PyObject *self, PyObject *args, PyObject *kwargs);
PyObject *pyyjson_Decode_sse2(PyObject *self, PyObject *args, PyObject *kwargs);


int CurrentSIMDFeatureLevel = -1;
PyCFunctionWithKeywords _pyyjson_encode_interface = NULL;
PyCFunctionWithKeywords _pyyjson_decode_interface = NULL;

PLATFORM_SIMD_LEVEL get_simd_feature(void) {
#    if PYYJSON_X86
    int info[4];
    cpuid_count(info, 7);
    int ebx = info[1];
    //
    if ((ebx & (1 << 16)) && (ebx & (1 << 30))) // AVX512F(16) and AVX512BW(30)
        return X86SIMDFeatureLevelAVX512;

    // check AVX2
    if (ebx & (1 << 5)) // AVX2(5)
        return X86SIMDFeatureLevelAVX2;

    // check SSE4.2, not used for now
    // cpuid(info, 1);
    // int ecx = info[2];
    // if (ecx & (1 << 20)) // SSE4.2(20)
    //     return X86SIMDFeatureLevelSSE4_2;

    //
    return X86SIMDFeatureLevelSSE2;
#    elif PYYJSON_AARCH
    return AArchSIMDFeatureLevelNEON;
#    endif
}

force_inline void _update_simd_features(void) {
    if (unlikely(CurrentSIMDFeatureLevel == -1)) {
        PLATFORM_SIMD_LEVEL simd_feature = get_simd_feature();
#    if PYYJSON_X86
        switch (simd_feature) {
            case X86SIMDFeatureLevelSSE2: {
                _pyyjson_encode_interface = pyyjson_Encode_sse2;
                _pyyjson_decode_interface = pyyjson_Decode_sse2;
                break;
            }
            // case X86SIMDFeatureLevelSSE4_2: {
            //     _pyyjson_encode_interface = pyyjson_Encode_sse4_2;
            //     _pyyjson_decode_interface = pyyjson_Decode_sse4_2;
            //     break;
            // }
            case X86SIMDFeatureLevelAVX2: {
                _pyyjson_encode_interface = pyyjson_Encode_avx2;
                _pyyjson_decode_interface = pyyjson_Decode_avx2;
                break;
            }
            case X86SIMDFeatureLevelAVX512: {
                _pyyjson_encode_interface = pyyjson_Encode_avx512;
                _pyyjson_decode_interface = pyyjson_Decode_avx512;
                break;
            }
            default: {
                assert(false);
            }
        }
#    elif PYYJSON_AARCH

#    endif
        // mark as ready
        CurrentSIMDFeatureLevel = (int)simd_feature;
    }
}

PyObject *pyyjson_Encode(PyObject *self, PyObject *args, PyObject *kwargs) {
    _update_simd_features();
    assert(_pyyjson_encode_interface);
    return _pyyjson_encode_interface(self, args, kwargs);
}

PyObject *pyyjson_Decode(PyObject *self, PyObject *args, PyObject *kwargs) {
    _update_simd_features();
    assert(_pyyjson_decode_interface);
    return _pyyjson_decode_interface(self, args, kwargs);
}

#endif


PyObject *pyyjson_print_current_features(PyObject *self, PyObject *args) {
    // TODO change to returning a dict with all build info
#if BUILD_MULTI_LIB
    _update_simd_features();
#    if PYYJSON_X86
    switch (CurrentSIMDFeatureLevel) {
        case X86SIMDFeatureLevelSSE2: {
            printf("SIMD: SSE2\n");
            break;
        }
        // case X86SIMDFeatureLevelSSE4_2: {
        //     printf("SIMD: SSE4.2\n");
        //     break;
        // }
        case X86SIMDFeatureLevelAVX2: {
            printf("SIMD: AVX2\n");
            break;
        }
        case X86SIMDFeatureLevelAVX512: {
            printf("SIMD: AVX512\n");
            break;
        }
        default: {
            printf("SIMD: Unknown\n");
            break;
        }
    }
#    elif PYYJSON_AARCH
    printf("SIMD: NEON\n");
#    endif
#else
#    if PYYJSON_X86
#        if COMPILE_SIMD_BITS == 512
    printf("SIMD: AVX512; MultiLib: False\n");
#        elif COMPILE_SIMD_BITS == 256
    printf("SIMD: AVX2; MultiLib: False\n");
// #    elif __SSE4_2__
//     printf("SIMD: SSE4.2; MultiLib: False\n");
#        else
    printf("SIMD: SSE2; MultiLib: False\n");
#        endif
#    elif PYYJSON_AARCH
    printf("SIMD: NEON; MultiLib: False\n");
#    endif
#endif
    Py_RETURN_NONE;
}

PyObject *pyyjson_get_current_features(PyObject *self, PyObject *args) {
    PyObject *ret = PyDict_New();
#if BUILD_MULTI_LIB
    _update_simd_features();
    PyDict_SetItemString(ret, "MultiLib", PyBool_FromLong(true));
#    if PYYJSON_X86
    switch (CurrentSIMDFeatureLevel) {
        case X86SIMDFeatureLevelSSE2: {
            PyDict_SetItemString(ret, "SIMD", PyUnicode_FromString("SSE2"));
            break;
        }
        // case X86SIMDFeatureLevelSSE4_2: {
        //     PyDict_SetItemString(ret, "SIMD", PyUnicode_FromString("SSE4.2"));
        //     break;
        // }
        case X86SIMDFeatureLevelAVX2: {
            PyDict_SetItemString(ret, "SIMD", PyUnicode_FromString("AVX2"));
            break;
        }
        case X86SIMDFeatureLevelAVX512: {
            PyDict_SetItemString(ret, "SIMD", PyUnicode_FromString("AVX512"));
            break;
        }
        default: {
            PyDict_SetItemString(ret, "SIMD", PyUnicode_FromString("Unknown"));
            break;
        }
    }
#    elif PYYJSON_AARCH
    PyDict_SetItemString(ret, "SIMD", PyUnicode_FromString("NEON"));
#    endif
#else
#    if PYYJSON_X86
    PyDict_SetItemString(ret, "MultiLib", PyBool_FromLong(false));

#        if COMPILE_SIMD_BITS == 512
    PyDict_SetItemString(ret, "SIMD", PyUnicode_FromString("AVX512"));
#        elif COMPILE_SIMD_BITS == 256
    PyDict_SetItemString(ret, "SIMD", PyUnicode_FromString("AVX2"));
// #    elif __SSE4_2__
//     PyDict_SetItemString(ret, "SIMD", PyUnicode_FromString("SSE4.2"));
#        else
    PyDict_SetItemString(ret, "SIMD", PyUnicode_FromString("SSE2"));
#        endif
#    elif PYYJSON_AARCH
    PyDict_SetItemString(ret, "SIMD", PyUnicode_FromString("NEON"));
#    endif
#endif
    return ret;
}
