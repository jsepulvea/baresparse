#ifndef BARESPARSE_BRSP_CSC_H
#define BARESPARSE_BRSP_CSC_H

#include <baresparse/brsp_status.h>
#include <baresparse/brsp_types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct brsp_csc_pattern {
    brsp_index row_count;
    brsp_index column_count;
    brsp_index nonzero_count;
    const brsp_index *column_offsets;
    const brsp_index *row_indices;
} brsp_csc_pattern;

typedef struct brsp_csc_matrix {
    brsp_csc_pattern pattern;
    const brsp_real *values;
} brsp_csc_matrix;

brsp_status brsp_csc_pattern_validate(const brsp_csc_pattern *pattern);
brsp_status brsp_csc_matrix_validate(const brsp_csc_matrix *matrix);

#ifdef __cplusplus
}
#endif

#endif
