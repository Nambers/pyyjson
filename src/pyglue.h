#ifndef PYYJSON_GLUE_H
#define PYYJSON_GLUE_H
#include <Python.h>
#include <assert.h>
#include <stdalign.h>
#include <stdbool.h>
#include <stdint.h>

#define PYYJSON_NO_OP (0)
#define PYYJSON_OP_NULL (1 << 0)
#define PYYJSON_OP_TRUE (1 << 1)
#define PYYJSON_OP_FALSE (1 << 2)
#define PYYJSON_OP_INT (1 << 3)
#define PYYJSON_OP_UINT (1 << 4)
#define PYYJSON_OP_FLOAT (1 << 5)
#define PYYJSON_OP_STRING (1 << 6)
#define PYYJSON_OP_ARRAY_END (1 << 7)
#define PYYJSON_OP_OBJECT_END (1 << 8)
#define PYYJSON_OP_INF (1 << 9)
#define PYYJSON_OP_NAN (1 << 10)
#define PYYJSON_OP_COUNT_MAX 11
#define PYYJSON_OP_MASK ((1 << PYYJSON_OP_COUNT_MAX) - 1)

#define PYYJSON_STRING_FLAG_ASCII 0
#define PYYJSON_STRING_FLAG_LATIN1 (1 << 11)
#define PYYJSON_STRING_FLAG_UCS2 (1 << 12)
#define PYYJSON_STRING_FLAG_UCS4 (1 << 13)
#define PYYJSON_STRING_FLAG_MASK (PYYJSON_STRING_FLAG_ASCII | PYYJSON_STRING_FLAG_LATIN1 | PYYJSON_STRING_FLAG_UCS2 | PYYJSON_STRING_FLAG_UCS4)

typedef uint32_t op_type;
#define PYYJSON_OP_HEAD pyyjson_op_base op_base;
#define PYYJSON_READ_OP(_op) (((pyyjson_op_base *) (_op))->op)
#define PYYJSON_WRITE_OP(_ptr, _code)               \
    do {                                            \
        ((pyyjson_op_base *) (_ptr))->op = (_code); \
    } while (0)

// size = 4
typedef struct pyyjson_op_base {
    op_type op;
} pyyjson_op_base;

// size = 8
typedef struct pyyjson_op {
    PYYJSON_OP_HEAD
    char pad[4];
} pyyjson_op;

// size = 16
typedef struct pyyjson_int_op {
    PYYJSON_OP_HEAD
    char pad[4];
    int64_t data;
} pyyjson_int_op;

// size = 16
typedef struct pyyjson_uint_op {
    PYYJSON_OP_HEAD
    char pad[4];
    uint64_t data;
} pyyjson_uint_op;

// size = 16
typedef struct pyyjson_float_op {
    PYYJSON_OP_HEAD
    char pad[4];
    double data;
} pyyjson_float_op;

// size = 16 / 24
typedef struct pyyjson_string_op {
    pyyjson_op op_base; // here we force the size of pyyjson_string_op to be 16 for 32bit platforms
    char *data;
    Py_ssize_t len;
} pyyjson_string_op;

// size = 8 / 16
typedef struct pyyjson_list_op {
    PYYJSON_OP_HEAD
    Py_ssize_t len;
} pyyjson_list_op;

// size = 8 / 16
typedef struct pyyjson_dict_op {
    PYYJSON_OP_HEAD
    Py_ssize_t len;
} pyyjson_dict_op;

// size = 8
typedef struct pyyjson_nan_op {
    PYYJSON_OP_HEAD
    bool sign;
} pyyjson_nan_op;

// size = 8
typedef struct pyyjson_inf_op {
    PYYJSON_OP_HEAD
    bool sign;
} pyyjson_inf_op;

PyObject *pyyjson_op_loads(pyyjson_op *op_sequence);

static_assert((sizeof(pyyjson_op) % sizeof(pyyjson_op)) == 0);
static_assert((sizeof(pyyjson_int_op) % sizeof(pyyjson_op)) == 0);
static_assert((sizeof(pyyjson_float_op) % sizeof(pyyjson_op)) == 0);
static_assert((sizeof(pyyjson_string_op) % sizeof(pyyjson_op)) == 0);
static_assert((sizeof(pyyjson_list_op) % sizeof(pyyjson_op)) == 0);
static_assert((sizeof(pyyjson_dict_op) % sizeof(pyyjson_op)) == 0);
static_assert((sizeof(pyyjson_nan_op) % sizeof(pyyjson_op)) == 0);
static_assert((sizeof(pyyjson_inf_op) % sizeof(pyyjson_op)) == 0);

#endif
