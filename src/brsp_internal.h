#ifndef BRSP_INTERNAL_H
#define BRSP_INTERNAL_H

#include <baresparse/brsp_types.h>

static inline int brsp_count_fits(uintmax_t count, brsp_size element_size) {
    return count <= (uintmax_t)(SIZE_MAX / element_size);
}

#endif
