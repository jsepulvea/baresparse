#include <baresparse/brsp_sparse_lu.h>

#include <math.h>

#include "brsp_internal.h"

brsp_status brsp_lu_analysis_workspace(brsp_index order, brsp_size *index_count) {
    if (index_count == NULL) {
        return BRSP_STATUS_INVALID_ARGUMENT;
    }
    *index_count = 0U;
    if (order == BRSP_INDEX_MAX || !brsp_count_fits(order, 2U * sizeof(brsp_index)) ||
        !brsp_count_fits(order, sizeof(brsp_real))) {
        return BRSP_STATUS_OVERFLOW;
    }
    *index_count = (brsp_size)order * 2U;
    return BRSP_STATUS_OK;
}

brsp_status brsp_lu_analysis_workspace_ordered(brsp_index order, brsp_size *index_count) {
    brsp_status status = brsp_lu_analysis_workspace(order, index_count);
    if (status != BRSP_STATUS_OK) {
        return status;
    }
    *index_count = 0U;
    if (!brsp_count_fits(order, 4U * sizeof(brsp_index))) {
        return BRSP_STATUS_OVERFLOW;
    }
    *index_count = (brsp_size)order * 4U;
    return BRSP_STATUS_OK;
}

static brsp_index brsp_lu_map(const brsp_index *map, brsp_index index) {
    return map == NULL ? index : map[index];
}

/* Validate both maps before using either as an index. Build the inverse row
 * map and the source -> destination permutation for the final solve vector. */
static brsp_status brsp_lu_prepare_ordering(brsp_index n, const brsp_lu_ordering *ordering,
                                            brsp_index *marks, brsp_index *inverse_rows,
                                            brsp_index *permutation) {
    brsp_index i;
    for (i = 0U; i < n; ++i) {
        marks[i] = 0U;
    }
    for (i = 0U; i < n; ++i) {
        const brsp_index row = brsp_lu_map(ordering->row_order, i);
        if (row >= n || marks[row] != 0U) {
            return BRSP_STATUS_INVALID_STRUCTURE;
        }
        marks[row] = 1U;
        inverse_rows[row] = i;
    }
    for (i = 0U; i < n; ++i) {
        marks[i] = 0U;
    }
    for (i = 0U; i < n; ++i) {
        const brsp_index col = brsp_lu_map(ordering->column_order, i);
        if (col >= n || marks[col] != 0U) {
            return BRSP_STATUS_INVALID_STRUCTURE;
        }
        marks[col] = 1U;
        permutation[brsp_lu_map(ordering->row_order, i)] = col;
    }
    return BRSP_STATUS_OK;
}

/* O(n) swaps, planned once. The same swaps move values from original row
 * positions to original column positions without extra solve workspace. */
static brsp_index brsp_lu_plan_swaps(brsp_index n, brsp_index *permutation, brsp_index *swaps) {
    brsp_index i, count = 0U;
    for (i = 0U; i < n; ++i) {
        while (permutation[i] != i) {
            const brsp_index j = permutation[i];
            if (swaps != NULL) {
                swaps[(brsp_size)count * 2U] = i;
                swaps[(brsp_size)count * 2U + 1U] = j;
            }
            permutation[i] = permutation[j];
            permutation[j] = j;
            ++count;
        }
    }
    return count;
}

/* Elimination graph path criterion in factor coordinates: (row,col) belongs
 * to LU iff the reordered matrix has a col -> row path with interior vertices
 * < min(row,col). Translate indices while traversing original CSC, so sizing
 * needs no previously stored L columns.
 * Deliberately favor bounded O(n) scratch over symbolic-analysis speed. */
static int brsp_lu_reachable(const brsp_csc_pattern *a, brsp_index row, brsp_index col,
                             const brsp_index *column_order, const brsp_index *inverse_rows,
                             brsp_index *marks, brsp_index *stack) {
    const brsp_index limit = row < col ? row : col;
    brsp_index i;
    brsp_index top = 0U;
    if (row == col) {
        return 1; /* Reserve pivots even when structurally absent. */
    }
    for (i = 0U; i < a->column_count; ++i) {
        marks[i] = 0U;
    }
    stack[top++] = col;
    marks[col] = 1U;
    while (top > 0U) {
        const brsp_index vertex = brsp_lu_map(column_order, stack[--top]);
        brsp_index p;
        for (p = a->column_offsets[vertex]; p < a->column_offsets[vertex + 1U]; ++p) {
            const brsp_index next = brsp_lu_map(inverse_rows, a->row_indices[p]);
            if (next == row) {
                return 1;
            }
            if (next < limit && marks[next] == 0U) {
                marks[next] = 1U;
                stack[top++] = next;
            }
        }
    }
    return 0;
}

brsp_status brsp_lu_plan_ordered(const brsp_csc_pattern *pattern, const brsp_lu_ordering *ordering,
                                 brsp_index *work, brsp_size work_capacity,
                                 brsp_lu_requirements *requirements) {
    brsp_lu_requirements req = {0};
    brsp_index col;
    brsp_size count = 0U;
    brsp_index *inverse_rows = NULL;
    brsp_index *permutation = NULL;
    brsp_status status;
    if (requirements == NULL) {
        return BRSP_STATUS_INVALID_ARGUMENT;
    }
    *requirements = req;
    if (pattern == NULL || pattern->row_count != pattern->column_count) {
        return BRSP_STATUS_INVALID_ARGUMENT;
    }
    if (ordering != NULL && ordering->order != pattern->column_count) {
        return BRSP_STATUS_INVALID_ARGUMENT;
    }
    status =
        ordering == NULL
            ? brsp_lu_analysis_workspace(pattern->column_count, &req.analysis_work_indices)
            : brsp_lu_analysis_workspace_ordered(pattern->column_count, &req.analysis_work_indices);
    if (status != BRSP_STATUS_OK) {
        return status;
    }
    status = brsp_csc_pattern_validate(pattern);
    if (status != BRSP_STATUS_OK) {
        return status;
    }
    if (work_capacity < req.analysis_work_indices) {
        return BRSP_STATUS_INSUFFICIENT_CAPACITY;
    }
    if (req.analysis_work_indices > 0U && work == NULL) {
        return BRSP_STATUS_INVALID_ARGUMENT;
    }
    if (ordering != NULL && pattern->column_count > 0U) {
        inverse_rows = work + (brsp_size)pattern->column_count * 2U;
        permutation = inverse_rows + pattern->column_count;
        status = brsp_lu_prepare_ordering(pattern->column_count, ordering, work, inverse_rows,
                                          permutation);
        if (status != BRSP_STATUS_OK) {
            return status;
        }
        req.solve_swap_count = brsp_lu_plan_swaps(pattern->column_count, permutation, NULL);
    }
    for (col = 0U; col < pattern->column_count; ++col) {
        brsp_index row;
        for (row = 0U; row < pattern->row_count; ++row) {
            if (brsp_lu_reachable(pattern, row, col,
                                  ordering == NULL ? NULL : ordering->column_order, inverse_rows,
                                  work, work + pattern->column_count)) {
                if (count == BRSP_INDEX_MAX || count == SIZE_MAX) {
                    return BRSP_STATUS_OVERFLOW;
                }
                ++count;
                if (row > col) {
                    ++req.l_nonzeros;
                } else {
                    ++req.u_nonzeros;
                }
            }
        }
    }
    /* 2*n+1 metadata indices: n+1 column offsets and n diagonal offsets. */
    req.symbolic_indices = (brsp_size)pattern->column_count * 2U + 1U;
    if (ordering != NULL) {
        const brsp_size maps = (brsp_size)pattern->column_count * 3U;
        const brsp_size swaps = (brsp_size)req.solve_swap_count * 2U;
        if (maps > SIZE_MAX - req.symbolic_indices) {
            return BRSP_STATUS_OVERFLOW;
        }
        req.symbolic_indices += maps;
        if (swaps > SIZE_MAX - req.symbolic_indices) {
            return BRSP_STATUS_OVERFLOW;
        }
        req.symbolic_indices += swaps;
    }
    if (count > SIZE_MAX - req.symbolic_indices) {
        return BRSP_STATUS_OVERFLOW;
    }
    req.symbolic_indices += count;
    if (req.symbolic_indices > SIZE_MAX / sizeof(brsp_index) ||
        count > SIZE_MAX / sizeof(brsp_real)) {
        return BRSP_STATUS_OVERFLOW;
    }
    req.factor_reals = count;
    req.numeric_work_reals = pattern->column_count;
    req.symbolic_bytes = req.symbolic_indices * sizeof(brsp_index);
    req.factor_bytes = count * sizeof(brsp_real);
    req.analysis_work_bytes = req.analysis_work_indices * sizeof(brsp_index);
    req.numeric_work_bytes = req.numeric_work_reals * sizeof(brsp_real);
    *requirements = req;
    return BRSP_STATUS_OK;
}

brsp_status brsp_lu_analyze_ordered(brsp_lu_symbolic *symbolic, const brsp_csc_pattern *pattern,
                                    const brsp_lu_ordering *ordering, brsp_index *storage,
                                    brsp_size storage_capacity, brsp_index *work,
                                    brsp_size work_capacity) {
    brsp_lu_requirements req;
    brsp_index *diagonal;
    brsp_index *rows;
    brsp_index *inverse_rows = NULL;
    brsp_index *column_order = NULL;
    brsp_index col;
    brsp_index count = 0U;
    brsp_status status;
    if (symbolic == NULL) {
        return BRSP_STATUS_INVALID_ARGUMENT;
    }
    symbolic->ready = 0;
    status = brsp_lu_plan_ordered(pattern, ordering, work, work_capacity, &req);
    if (status != BRSP_STATUS_OK) {
        return status;
    }
    if (storage_capacity < req.symbolic_indices) {
        return BRSP_STATUS_INSUFFICIENT_CAPACITY;
    }
    if (storage == NULL) {
        return BRSP_STATUS_INVALID_ARGUMENT;
    }
    diagonal = storage + (brsp_size)pattern->column_count + 1U;
    rows = diagonal + pattern->column_count;
    symbolic->row_order = NULL;
    symbolic->column_order = NULL;
    symbolic->inverse_row_order = NULL;
    symbolic->solve_swaps = NULL;
    symbolic->solve_swap_count = req.solve_swap_count;
    if (ordering != NULL && pattern->column_count > 0U) {
        const brsp_index n = pattern->column_count;
        brsp_index *row_order = rows + req.factor_reals;
        brsp_index *swaps;
        brsp_index *permutation = work + (brsp_size)n * 3U;
        column_order = row_order + n;
        inverse_rows = column_order + n;
        swaps = inverse_rows + n;
        /* Planning already validated both maps. Rebuild the consumed swap map. */
        (void)brsp_lu_prepare_ordering(n, ordering, work, inverse_rows, permutation);
        for (col = 0U; col < n; ++col) {
            row_order[col] = brsp_lu_map(ordering->row_order, col);
            column_order[col] = brsp_lu_map(ordering->column_order, col);
        }
        (void)brsp_lu_plan_swaps(n, permutation, swaps);
        symbolic->row_order = row_order;
        symbolic->column_order = column_order;
        symbolic->inverse_row_order = inverse_rows;
        symbolic->solve_swaps = swaps;
    }
    storage[0] = 0U;
    for (col = 0U; col < pattern->column_count; ++col) {
        brsp_index row;
        for (row = 0U; row < pattern->row_count; ++row) {
            if (brsp_lu_reachable(pattern, row, col, column_order, inverse_rows, work,
                                  work + pattern->column_count)) {
                if (row == col) {
                    diagonal[col] = count;
                }
                rows[count++] = row;
            }
        }
        storage[col + 1U] = count;
    }
    symbolic->pattern = *pattern;
    symbolic->column_offsets = storage;
    symbolic->diagonal_offsets = diagonal;
    symbolic->row_indices = rows;
    symbolic->factor_nonzeros = count;
    symbolic->ready = 1;
    return BRSP_STATUS_OK;
}

brsp_status brsp_lu_plan(const brsp_csc_pattern *pattern, brsp_index *work, brsp_size work_capacity,
                         brsp_lu_requirements *requirements) {
    return brsp_lu_plan_ordered(pattern, NULL, work, work_capacity, requirements);
}

brsp_status brsp_lu_analyze(brsp_lu_symbolic *symbolic, const brsp_csc_pattern *pattern,
                            brsp_index *storage, brsp_size storage_capacity, brsp_index *work,
                            brsp_size work_capacity) {
    return brsp_lu_analyze_ordered(symbolic, pattern, NULL, storage, storage_capacity, work,
                                   work_capacity);
}

brsp_status brsp_lu_numeric_init(brsp_lu_numeric *numeric, const brsp_lu_symbolic *symbolic,
                                 brsp_real *factors, brsp_size factor_capacity) {
    if (numeric == NULL) {
        return BRSP_STATUS_INVALID_ARGUMENT;
    }
    numeric->valid = 0;
    numeric->symbolic = NULL;
    numeric->factors = NULL;
    numeric->capacity = 0U;
    if (symbolic == NULL || !symbolic->ready) {
        return BRSP_STATUS_INVALID_ARGUMENT;
    }
    if (factor_capacity < symbolic->factor_nonzeros) {
        return BRSP_STATUS_INSUFFICIENT_CAPACITY;
    }
    if (symbolic->factor_nonzeros > 0U && factors == NULL) {
        return BRSP_STATUS_INVALID_ARGUMENT;
    }
    numeric->symbolic = symbolic;
    numeric->factors = factors;
    numeric->capacity = factor_capacity;
    return BRSP_STATUS_OK;
}

brsp_status brsp_lu_refactor(brsp_lu_numeric *numeric, const brsp_real *values,
                             brsp_size value_count, brsp_real pivot_tolerance, brsp_real *work,
                             brsp_size work_capacity) {
    const brsp_lu_symbolic *s;
    brsp_index col;
    brsp_index p;
    if (numeric == NULL) {
        return BRSP_STATUS_INVALID_ARGUMENT;
    }
    numeric->valid = 0;
    s = numeric->symbolic;
    if (s == NULL || !s->ready || !isfinite(pivot_tolerance) || pivot_tolerance < 0.0F) {
        return BRSP_STATUS_INVALID_ARGUMENT;
    }
    if (value_count != s->pattern.nonzero_count) {
        return BRSP_STATUS_INVALID_ARGUMENT;
    }
    if (work_capacity < s->pattern.column_count || numeric->capacity < s->factor_nonzeros) {
        return BRSP_STATUS_INSUFFICIENT_CAPACITY;
    }
    if ((value_count > 0U && values == NULL) ||
        (s->pattern.column_count > 0U && (work == NULL || numeric->factors == NULL))) {
        return BRSP_STATUS_INVALID_ARGUMENT;
    }
    for (p = 0U; p < s->pattern.nonzero_count; ++p) {
        if (!isfinite(values[p])) {
            return BRSP_STATUS_NONFINITE;
        }
    }
    for (col = 0U; col < s->pattern.column_count; ++col) {
        const brsp_index diagonal = s->diagonal_offsets[col];
        const brsp_index original_col = brsp_lu_map(s->column_order, col);
        brsp_index row;
        brsp_real pivot;
        for (row = 0U; row < s->pattern.row_count; ++row) {
            work[row] = 0.0F;
        }
        for (p = s->pattern.column_offsets[original_col];
             p < s->pattern.column_offsets[original_col + 1U]; ++p) {
            work[brsp_lu_map(s->inverse_row_order, s->pattern.row_indices[p])] = values[p];
        }
        /* Left-looking updates use only the precomputed U and L layouts. */
        for (p = s->column_offsets[col]; p < diagonal; ++p) {
            const brsp_index k = s->row_indices[p];
            const brsp_real upper = work[k];
            brsp_index q;
            for (q = s->diagonal_offsets[k] + 1U; q < s->column_offsets[k + 1U]; ++q) {
                work[s->row_indices[q]] -= numeric->factors[q] * upper;
            }
        }
        pivot = work[col];
        if (!isfinite(pivot)) {
            return BRSP_STATUS_NONFINITE;
        }
        if (fabsf(pivot) <= pivot_tolerance) {
            return BRSP_STATUS_PIVOT_REJECTED;
        }
        for (p = s->column_offsets[col]; p < s->column_offsets[col + 1U]; ++p) {
            const brsp_real value =
                p > diagonal ? work[s->row_indices[p]] / pivot : work[s->row_indices[p]];
            if (!isfinite(value)) {
                return BRSP_STATUS_NONFINITE;
            }
            numeric->factors[p] = value;
        }
    }
    numeric->valid = 1;
    return BRSP_STATUS_OK;
}

brsp_status brsp_lu_solve(const brsp_lu_numeric *numeric, const brsp_real *rhs, brsp_size rhs_count,
                          brsp_real *solution, brsp_size solution_capacity) {
    const brsp_lu_symbolic *s;
    brsp_index col;
    if (numeric == NULL) {
        return BRSP_STATUS_INVALID_ARGUMENT;
    }
    if (!numeric->valid || numeric->symbolic == NULL || !numeric->symbolic->ready) {
        return BRSP_STATUS_NOT_FACTORED;
    }
    s = numeric->symbolic;
    if (rhs_count != s->pattern.column_count) {
        return BRSP_STATUS_INVALID_ARGUMENT;
    }
    if (solution_capacity < rhs_count) {
        return BRSP_STATUS_INSUFFICIENT_CAPACITY;
    }
    if (rhs_count > 0U && (rhs == NULL || solution == NULL)) {
        return BRSP_STATUS_INVALID_ARGUMENT;
    }
    for (col = 0U; col < s->pattern.column_count; ++col) {
        if (!isfinite(rhs[col])) {
            return BRSP_STATUS_NONFINITE;
        }
    }
    for (col = 0U; col < s->pattern.column_count; ++col) {
        solution[col] = rhs[col];
    }
    for (col = 0U; col < s->pattern.column_count; ++col) {
        brsp_index p;
        for (p = s->diagonal_offsets[col] + 1U; p < s->column_offsets[col + 1U]; ++p) {
            solution[brsp_lu_map(s->row_order, s->row_indices[p])] -=
                numeric->factors[p] * solution[brsp_lu_map(s->row_order, col)];
        }
    }
    for (col = s->pattern.column_count; col > 0U; --col) {
        const brsp_index j = col - 1U;
        const brsp_index original_row = brsp_lu_map(s->row_order, j);
        brsp_index p;
        solution[original_row] /= numeric->factors[s->diagonal_offsets[j]];
        if (!isfinite(solution[original_row])) {
            return BRSP_STATUS_NONFINITE;
        }
        for (p = s->column_offsets[j]; p < s->diagonal_offsets[j]; ++p) {
            solution[brsp_lu_map(s->row_order, s->row_indices[p])] -=
                numeric->factors[p] * solution[original_row];
        }
    }
    for (col = 0U; col < s->solve_swap_count; ++col) {
        const brsp_index i = s->solve_swaps[(brsp_size)col * 2U];
        const brsp_index j = s->solve_swaps[(brsp_size)col * 2U + 1U];
        const brsp_real value = solution[i];
        solution[i] = solution[j];
        solution[j] = value;
    }
    return BRSP_STATUS_OK;
}
