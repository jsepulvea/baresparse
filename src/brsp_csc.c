#include <baresparse/brsp_csc.h>

#include "brsp_internal.h"

brsp_status brsp_csc_pattern_validate(const brsp_csc_pattern *pattern) {
    brsp_index column;
    brsp_index position;

    if (pattern == NULL || pattern->column_offsets == NULL) {
        return BRSP_STATUS_INVALID_ARGUMENT;
    }

    if (pattern->column_count == BRSP_INDEX_MAX ||
        !brsp_count_fits((uintmax_t)pattern->column_count + 1U, sizeof(brsp_index)) ||
        !brsp_count_fits(pattern->nonzero_count, sizeof(brsp_index))) {
        return BRSP_STATUS_OVERFLOW;
    }

    if (pattern->nonzero_count > 0U && pattern->row_indices == NULL) {
        return BRSP_STATUS_INVALID_ARGUMENT;
    }

    if (pattern->column_offsets[0] != 0U) {
        return BRSP_STATUS_INVALID_STRUCTURE;
    }

    for (column = 0U; column < pattern->column_count; ++column) {
        const brsp_index begin = pattern->column_offsets[column];
        const brsp_index end = pattern->column_offsets[column + 1U];

        if (begin > end || end > pattern->nonzero_count) {
            return BRSP_STATUS_INVALID_STRUCTURE;
        }

        for (position = begin; position < end; ++position) {
            if (pattern->row_indices[position] >= pattern->row_count) {
                return BRSP_STATUS_INVALID_STRUCTURE;
            }
            if (position > begin &&
                pattern->row_indices[position - 1U] >= pattern->row_indices[position]) {
                return BRSP_STATUS_INVALID_STRUCTURE;
            }
        }
    }

    if (pattern->column_offsets[pattern->column_count] != pattern->nonzero_count) {
        return BRSP_STATUS_INVALID_STRUCTURE;
    }

    return BRSP_STATUS_OK;
}

brsp_status brsp_csc_matrix_validate(const brsp_csc_matrix *matrix) {
    brsp_status status;

    if (matrix == NULL) {
        return BRSP_STATUS_INVALID_ARGUMENT;
    }

    status = brsp_csc_pattern_validate(&matrix->pattern);
    if (status != BRSP_STATUS_OK) {
        return status;
    }

    if (matrix->pattern.nonzero_count > 0U && matrix->values == NULL) {
        return BRSP_STATUS_INVALID_ARGUMENT;
    }

    return BRSP_STATUS_OK;
}
