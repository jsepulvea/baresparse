#ifndef BARESPARSE_BRSP_TYPES_H
#define BARESPARSE_BRSP_TYPES_H

#include <limits.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef float brsp_real;
typedef uint32_t brsp_index;
typedef size_t brsp_size;

#define BRSP_INDEX_MAX UINT32_MAX

#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
_Static_assert(sizeof(brsp_real) * CHAR_BIT == 32, "BareSparse requires a 32-bit float");
_Static_assert(sizeof(brsp_index) * CHAR_BIT == 32, "BareSparse requires a 32-bit index");
#endif

#ifdef __cplusplus
}
#endif

#endif
