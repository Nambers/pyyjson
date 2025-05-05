#ifdef PYYJSON_CLANGD_DUMMY
#    ifndef COMPILE_READ_UCS_LEVEL
#        define COMPILE_READ_UCS_LEVEL 1
#    endif
#endif

#include "commondef/r_in.inl.h"

force_inline bool cmpeq_2chars(const _src_t *src, const _src_t *_template, const _src_t *end) {
    return src + 2 <= end && 0 == memcmp(src, _template, 2 * sizeof(_src_t));
}

#include "commondef/r_out.inl.h"
