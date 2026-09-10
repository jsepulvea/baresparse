#include <baresparse/brsp_dense_lu.h>

#include "brsp_internal.h"
#include <math.h>

static brsp_real brsp_real_abs(brsp_real value) {
    return value < 0.0F ? -value : value;
}

static void brsp_dense_swap_rows(brsp_real *matrix, brsp_index leading_dimension,
                                 brsp_index column_count, brsp_index first_row,
                                 brsp_index second_row) {
    brsp_index column;

    if (first_row == second_row) {
        return;
    }

    for (column = 0U; column < column_count; ++column) {
        const brsp_size first_offset = (brsp_size)first_row * leading_dimension + column;
        const brsp_size second_offset = (brsp_size)second_row * leading_dimension + column;
        const brsp_real temporary = matrix[first_offset];
        matrix[first_offset] = matrix[second_offset];
        matrix[second_offset] = temporary;
    }
}

brsp_status brsp_dense_lu_factor(brsp_index order, brsp_real *matrix, brsp_index leading_dimension,
                                 brsp_index *pivots, brsp_real pivot_tolerance) {
    brsp_index pivot_column;

    if (!isfinite(pivot_tolerance) || pivot_tolerance < 0.0F) {
        return BRSP_STATUS_INVALID_ARGUMENT;
    }

    if (order == 0U) {
        return BRSP_STATUS_OK;
    }

    if (matrix == NULL || pivots == NULL || leading_dimension < order) {
        return BRSP_STATUS_INVALID_ARGUMENT;
    }

    if (!brsp_count_fits(leading_dimension, sizeof(brsp_real)) ||
        !brsp_count_fits(order, (brsp_size)leading_dimension * sizeof(brsp_real))) {
        return BRSP_STATUS_OVERFLOW;
    }
    for (pivot_column = 0U; pivot_column < order; ++pivot_column) {
        brsp_index column;
        for (column = 0U; column < order; ++column) {
            if (!isfinite(matrix[(brsp_size)pivot_column * leading_dimension + column])) {
                return BRSP_STATUS_NONFINITE;
            }
        }
    }

    for (pivot_column = 0U; pivot_column < order; ++pivot_column) {
        brsp_index row;
        brsp_index pivot_row = pivot_column;
        brsp_real pivot_magnitude =
            brsp_real_abs(matrix[(brsp_size)pivot_column * leading_dimension + pivot_column]);

        for (row = pivot_column + 1U; row < order; ++row) {
            const brsp_real candidate =
                brsp_real_abs(matrix[(brsp_size)row * leading_dimension + pivot_column]);
            if (candidate > pivot_magnitude) {
                pivot_magnitude = candidate;
                pivot_row = row;
            }
        }

        if (!isfinite(pivot_magnitude)) {
            return BRSP_STATUS_NONFINITE;
        }
        if (pivot_magnitude <= pivot_tolerance) {
            return BRSP_STATUS_SINGULAR;
        }

        pivots[pivot_column] = pivot_row;
        brsp_dense_swap_rows(matrix, leading_dimension, order, pivot_column, pivot_row);

        for (row = pivot_column + 1U; row < order; ++row) {
            brsp_index column;
            const brsp_size multiplier_offset = (brsp_size)row * leading_dimension + pivot_column;
            matrix[multiplier_offset] /=
                matrix[(brsp_size)pivot_column * leading_dimension + pivot_column];
            if (!isfinite(matrix[multiplier_offset])) {
                return BRSP_STATUS_NONFINITE;
            }

            for (column = pivot_column + 1U; column < order; ++column) {
                matrix[(brsp_size)row * leading_dimension + column] -=
                    matrix[multiplier_offset] *
                    matrix[(brsp_size)pivot_column * leading_dimension + column];
                if (!isfinite(matrix[(brsp_size)row * leading_dimension + column])) {
                    return BRSP_STATUS_NONFINITE;
                }
            }
        }
    }

    return BRSP_STATUS_OK;
}

brsp_status brsp_dense_lu_solve(brsp_index order, const brsp_real *factor,
                                brsp_index leading_dimension, const brsp_index *pivots,
                                const brsp_real *right_hand_side, brsp_real *solution) {
    brsp_index row;

    if (order == 0U) {
        return BRSP_STATUS_OK;
    }

    if (factor == NULL || pivots == NULL || right_hand_side == NULL || solution == NULL ||
        leading_dimension < order) {
        return BRSP_STATUS_INVALID_ARGUMENT;
    }

    if (!brsp_count_fits(leading_dimension, sizeof(brsp_real)) ||
        !brsp_count_fits(order, (brsp_size)leading_dimension * sizeof(brsp_real))) {
        return BRSP_STATUS_OVERFLOW;
    }
    for (row = 0U; row < order; ++row) {
        if (!isfinite(right_hand_side[row])) {
            return BRSP_STATUS_NONFINITE;
        }
    }
    for (row = 0U; row < order; ++row) {
        solution[row] = right_hand_side[row];
    }

    for (row = 0U; row < order; ++row) {
        const brsp_index pivot_row = pivots[row];
        if (pivot_row >= order) {
            return BRSP_STATUS_INVALID_STRUCTURE;
        }
        if (pivot_row != row) {
            const brsp_real temporary = solution[row];
            solution[row] = solution[pivot_row];
            solution[pivot_row] = temporary;
        }
    }

    for (row = 0U; row < order; ++row) {
        brsp_index column;
        for (column = 0U; column < row; ++column) {
            solution[row] -= factor[(brsp_size)row * leading_dimension + column] * solution[column];
        }
    }

    for (row = order; row > 0U; --row) {
        brsp_index column;
        const brsp_index current_row = row - 1U;
        const brsp_real diagonal = factor[(brsp_size)current_row * leading_dimension + current_row];

        if (diagonal == 0.0F) {
            return BRSP_STATUS_SINGULAR;
        }

        for (column = current_row + 1U; column < order; ++column) {
            solution[current_row] -=
                factor[(brsp_size)current_row * leading_dimension + column] * solution[column];
        }
        solution[current_row] /= diagonal;
        if (!isfinite(solution[current_row])) {
            return BRSP_STATUS_NONFINITE;
        }
    }

    return BRSP_STATUS_OK;
}
