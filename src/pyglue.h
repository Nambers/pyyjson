#ifndef PYYJSON_GLUE_H
#define PYYJSON_GLUE_H
#include <Python.h>
#include <assert.h>
#include <stdalign.h>


#define PYYJSON_NO_OP (0)
#define PYYJSON_OP_NULL (1 << 0)
#define PYYJSON_OP_TRUE (1 << 1)
#define PYYJSON_OP_FALSE (1 << 2)
#define PYYJSON_OP_INT (1 << 3)
#define PYYJSON_OP_FLOAT (1 << 4)
#define PYYJSON_OP_STRING (1 << 5)
#define PYYJSON_OP_ARRAY_END (1 << 6)
#define PYYJSON_OP_OBJECT_END (1 << 7)
#define PYYJSON_OP_COUNT_MAX 8
#define PYYJSON_OP_MASK ((1 << PYYJSON_OP_COUNT_MAX) - 1)

#define PYYJSON_STRING_FLAG_ASCII 0         /* 000__1_____ */
#define PYYJSON_STRING_FLAG_LATIN1 (1 << 8) /* 001__1_____ */
#define PYYJSON_STRING_FLAG_UCS2 (1 << 9)   /* 010__1_____ */
#define PYYJSON_STRING_FLAG_UCS4 (1 << 10)  /* 100__1_____ */
#define PYYJSON_STRING_FLAG_MASK (PYYJSON_STRING_FLAG_ASCII | PYYJSON_STRING_FLAG_LATIN1 | PYYJSON_STRING_FLAG_UCS2 | PYYJSON_STRING_FLAG_UCS4)

typedef uint32_t op_type;
typedef struct pyyjson_op {
    op_type op;
} pyyjson_op;
#define PYYJSON_OP_HEAD pyyjson_op op_base;


typedef struct pyyjson_int_op {
    PYYJSON_OP_HEAD
    long long data;
} pyyjson_int_op;

typedef struct pyyjson_float_op {
    PYYJSON_OP_HEAD
    double data;
} pyyjson_float_op;

typedef struct pyyjson_string_op {
    PYYJSON_OP_HEAD
    char *data;
    Py_ssize_t len;
} pyyjson_string_op;

typedef struct pyyjson_list_op {
    PYYJSON_OP_HEAD
    Py_ssize_t len;
} pyyjson_list_op;

typedef struct pyyjson_dict_op {
    PYYJSON_OP_HEAD
    Py_ssize_t len;
} pyyjson_dict_op;

PyObject *pyyjson_op_loads(pyyjson_op *op_sequence);

#endif
