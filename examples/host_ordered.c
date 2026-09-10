#include <baresparse/brsp.h>
#include <math.h>
#include <stdio.h>

#define BRSP_TRY(call)                                                                             \
    do {                                                                                           \
        const brsp_status status = (call);                                                         \
        if (status != BRSP_STATUS_OK) {                                                            \
            (void)fprintf(stderr, "%s: %s\n", #call, brsp_status_string(status));                  \
            return 1;                                                                              \
        }                                                                                          \
    } while (0)

int main(void) {
    static const brsp_index offsets[] = {0, 1, 2, 3};
    static const brsp_index rows[] = {2, 0, 1};
    /* B(i,j) = A(row_order[i], column_order[j]) is diagonal. A has no
     * diagonal entries; identity ordering would reject its first pivot. */
    static const brsp_index row_order[] = {1, 2, 0};
    static const brsp_index column_order[] = {2, 0, 1};
    const brsp_lu_ordering ordering = {3, row_order, column_order};
    const brsp_csc_pattern pattern = {3, 3, 3, offsets, rows};
    brsp_lu_symbolic symbolic = {0};
    brsp_lu_numeric numeric = {0};
    brsp_lu_requirements req;
    /* Application-owned storage. These capacities fit this hand-checkable
     * pattern; a hosted caller can instead allocate the queried sizes. */
    brsp_index analysis_work[12], symbolic_storage[23];
    brsp_real factors[3], numeric_work[3], x[3];
    brsp_real values[] = {3, 4, 2};
    brsp_size analysis_count;
    brsp_index step;
    BRSP_TRY(brsp_lu_analysis_workspace_ordered(3, &analysis_count));
    if (analysis_count > 12)
        return 1;
    BRSP_TRY(brsp_lu_plan_ordered(&pattern, &ordering, analysis_work, 12, &req));
    if (req.symbolic_indices > 23 || req.factor_reals > 3 || req.numeric_work_reals > 3)
        return 1;
    BRSP_TRY(brsp_lu_analyze_ordered(&symbolic, &pattern, &ordering, symbolic_storage, 23,
                                     analysis_work, 12));
    BRSP_TRY(brsp_lu_numeric_init(&numeric, &symbolic, factors, 3));
    for (step = 0; step < 4; ++step) {
        brsp_real rhs[3] = {0};
        brsp_index col;
        values[0] = 3.0F + (brsp_real)step;
        for (col = 0; col < 3; ++col) {
            brsp_index p;
            for (p = offsets[col]; p < offsets[col + 1]; ++p)
                rhs[rows[p]] += values[p] * (brsp_real)(col + 1);
        }
        BRSP_TRY(brsp_lu_refactor(&numeric, values, 3, 1.0e-7F, numeric_work, 3));
        /* In-place solve still returns unknowns in original column order. */
        for (col = 0; col < 3; ++col)
            x[col] = rhs[col];
        BRSP_TRY(brsp_lu_solve(&numeric, x, 3, x, 3));
        for (col = 0; col < 3; ++col)
            if (fabsf(x[col] - (brsp_real)(col + 1)) > 1.0e-5F)
                return 1;
        (void)printf("step %lu: x = %.6g %.6g %.6g\n", (unsigned long)step, (double)x[0],
                     (double)x[1], (double)x[2]);
    }
    (void)printf("Stored factors: %lu L entries (unit diagonal implicit), %lu U entries\n",
                 (unsigned long)req.l_nonzeros, (unsigned long)req.u_nonzeros);
    return 0;
}
