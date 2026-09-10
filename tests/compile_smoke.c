#include <baresparse/brsp.h>

brsp_status brsp_compile_smoke(const brsp_csc_matrix *matrix);

brsp_status brsp_compile_smoke(const brsp_csc_matrix *matrix) {
    return brsp_csc_matrix_validate(matrix);
}

/* Also compile the public ordered lifecycle on every target compiler. */
brsp_status brsp_compile_ordered_smoke(void);

brsp_status brsp_compile_ordered_smoke(void) {
    const brsp_index offsets[] = {0, 1, 2}, rows[] = {1, 0}, row_order[] = {1, 0};
    const brsp_real values[] = {2, 3};
    const brsp_csc_pattern pattern = {2, 2, 2, offsets, rows};
    const brsp_lu_ordering ordering = {2, row_order, NULL};
    brsp_lu_symbolic symbolic = {0};
    brsp_lu_numeric numeric = {0};
    brsp_lu_requirements req;
    brsp_size count;
    brsp_index iw[8], storage[15];
    brsp_real factors[2], work[2], rhs[] = {6, 2};
    brsp_status status = brsp_lu_analysis_workspace_ordered(2, &count);
    if (status != BRSP_STATUS_OK)
        return status;
    if (count > 8)
        return BRSP_STATUS_INSUFFICIENT_CAPACITY;
    status = brsp_lu_plan_ordered(&pattern, &ordering, iw, count, &req);
    if (status != BRSP_STATUS_OK)
        return status;
    if (req.symbolic_indices > 15 || req.factor_reals > 2 || req.numeric_work_reals > 2)
        return BRSP_STATUS_INSUFFICIENT_CAPACITY;
    status = brsp_lu_analyze_ordered(&symbolic, &pattern, &ordering, storage, 15, iw, count);
    if (status != BRSP_STATUS_OK)
        return status;
    status = brsp_lu_numeric_init(&numeric, &symbolic, factors, 2);
    if (status != BRSP_STATUS_OK)
        return status;
    status = brsp_lu_refactor(&numeric, values, 2, 0, work, 2);
    if (status != BRSP_STATUS_OK)
        return status;
    return brsp_lu_solve(&numeric, rhs, 2, rhs, 2);
}
