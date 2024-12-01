#ifndef PYYJSON_DECODE_H
#define PYYJSON_DECODE_H
#include "pyyjson_config.h"
#include <assert.h>

// hot spot:
// string -> float,int->array,dict->null,false,true. least: uint, nan, inf
#define PYYJSON_NO_OP (0)
#define PYYJSON_OP_STRING (1)
#define PYYJSON_OP_NUMBER (2)
#define PYYJSON_OP_CONTAINER (3)
#define PYYJSON_OP_CONSTANTS (4)
#define PYYJSON_OP_NAN_INF (5)
// mask: 7 = (1 << 3) - 1, to cover 0~5
#define PYYJSON_OP_MASK (7)
// PYYJSON_OP_BITCOUNT_MAX = 3, to cover 0~5
#define PYYJSON_OP_BITCOUNT_MAX (3)
// higher start: 8
#define PYYJSON_OP_HIGHER_START (1 << PYYJSON_OP_BITCOUNT_MAX)
//string flags ~~
#define PYYJSON_STRING_FLAG_ASCII (0 << PYYJSON_OP_BITCOUNT_MAX)
#define PYYJSON_STRING_FLAG_LATIN1 (1 << PYYJSON_OP_BITCOUNT_MAX)
#define PYYJSON_STRING_FLAG_UCS2 (2 << PYYJSON_OP_BITCOUNT_MAX)
#define PYYJSON_STRING_FLAG_UCS4 (3 << PYYJSON_OP_BITCOUNT_MAX)
#define PYYJSON_STRING_FLAG_UCS_TYPE_MASK (3 << PYYJSON_OP_BITCOUNT_MAX)
// is key ~~
#define PYYJSON_STRING_FLAG_OBJ_KEY (4 << PYYJSON_OP_BITCOUNT_MAX)
// num flags ~~
#define PYYJSON_NUM_FLAG_FLOAT (0 << PYYJSON_OP_BITCOUNT_MAX)
#define PYYJSON_NUM_FLAG_INT (1 << PYYJSON_OP_BITCOUNT_MAX)
#define PYYJSON_NUM_FLAG_UINT (2 << PYYJSON_OP_BITCOUNT_MAX)
#define PYYJSON_NUM_FLAG_MASK (3 << PYYJSON_OP_BITCOUNT_MAX)
// container flags ~~
#define PYYJSON_CONTAINER_FLAG_ARRAY (0 << PYYJSON_OP_BITCOUNT_MAX)
#define PYYJSON_CONTAINER_FLAG_DICT (1 << PYYJSON_OP_BITCOUNT_MAX)
#define PYYJSON_CONTAINER_FLAG_MASK (1 << PYYJSON_OP_BITCOUNT_MAX)
// constants flags
#define PYYJSON_CONSTANTS_FLAG_NULL (0 << PYYJSON_OP_BITCOUNT_MAX)
#define PYYJSON_CONSTANTS_FLAG_FALSE (1 << PYYJSON_OP_BITCOUNT_MAX)
#define PYYJSON_CONSTANTS_FLAG_TRUE (2 << PYYJSON_OP_BITCOUNT_MAX)
#define PYYJSON_CONSTANTS_FLAG_MASK (3 << PYYJSON_OP_BITCOUNT_MAX)
// nan inf flags
#define PYYJSON_NAN_INF_FLAG_NAN (0 << PYYJSON_OP_BITCOUNT_MAX)
#define PYYJSON_NAN_INF_FLAG_INF (1 << PYYJSON_OP_BITCOUNT_MAX)
#define PYYJSON_NAN_INF_FLAG_SIGNED (2 << PYYJSON_OP_BITCOUNT_MAX)
#define PYYJSON_NAN_INF_FLAG_MASK_WITHOUT_SIGNED (1 << PYYJSON_OP_BITCOUNT_MAX)
//interpret flags
#define PYYJSON_MATCH_FLAG(x) case ((x) >> PYYJSON_OP_BITCOUNT_MAX)
#define PYYJSON_GET_FLAG(x, mask) (((x) & (mask)) >> PYYJSON_OP_BITCOUNT_MAX)
// end flags
typedef uint32_t op_type;
#define PYYJSON_OP_HEAD pyyjson_op_base op_base;
#define PYYJSON_READ_OP(_op) (((pyyjson_op_base *) (_op))->op)
#define PYYJSON_WRITE_OP(_ptr, _code)               \
    do {                                            \
        ((pyyjson_op_base *) (_ptr))->op = (_code); \
    } while (0)


#ifdef PYYJSON_64BIT
#define PYYJSON_OP_PADDING char pad[4];
#else
#define PYYJSON_OP_PADDING
#endif

// size = 4. This should not be used directly
typedef struct pyyjson_op_base {
    op_type op;
} pyyjson_op_base;

// size = 4 / 8
typedef struct pyyjson_op {
    PYYJSON_OP_HEAD
    PYYJSON_OP_PADDING
} pyyjson_op;

// size = 12 / 16
typedef struct pyyjson_number_op {
    PYYJSON_OP_HEAD
    union {
        int64_t i;
        uint64_t u;
        double f;
    } data;
} pyyjson_number_op;

// size = 12 / 24
typedef struct pyyjson_string_op {
    PYYJSON_OP_HEAD
    char *data;
    Py_ssize_t len;
} pyyjson_string_op;

// size = 8 / 16
typedef struct pyyjson_container_op {
    PYYJSON_OP_HEAD
    Py_ssize_t len;
} pyyjson_container_op;

PyObject *pyyjson_op_loads(pyyjson_op *op_sequence, size_t obj_stack_maxsize);

extern PyObject *JSONDecodeError;

typedef PyObject *pyyjson_cache_type;
extern pyyjson_cache_type AssociativeKeyCache[PYYJSON_KEY_CACHE_SIZE];

// static assertions
static_assert((sizeof(pyyjson_number_op) % sizeof(pyyjson_op)) == 0, "size of pyyjson_number_op  must be multiple of size of pyyjson_op");
static_assert((sizeof(pyyjson_string_op) % sizeof(pyyjson_op)) == 0, "size of pyyjson_string_op  must be multiple of size of pyyjson_op");
static_assert((sizeof(pyyjson_container_op) % sizeof(pyyjson_op)) == 0, "size of pyyjson_container_op  must be multiple of size of pyyjson_op");
static_assert(sizeof(long long) == 8, "size of long long must be 8 bytes");
static_assert(sizeof(unsigned long long) == 8, "size of unsigned long long must be 8 bytes");
static_assert(sizeof(double) == 8, "size of double must be 8 bytes");

#endif // PYYJSON_DECODE_H
