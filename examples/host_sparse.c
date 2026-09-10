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
    static const brsp_index offsets[] = {0, 2, 4, 6};
    static const brsp_index rows[] = {0, 1, 1, 2, 0, 2};
    const brsp_csc_pattern pattern = {3, 3, 6, offsets, rows};
    brsp_lu_symbolic symbolic = {0};
    brsp_lu_numeric numeric = {0};
    brsp_lu_requirements req;
    /* Application-owned storage. These capacities fit this hand-checkable
     * pattern; a hosted caller can instead allocate the queried sizes. */
    brsp_index analysis_work[6], symbolic_storage[14];
    brsp_real factors[7], numeric_work[3], x[3];
    brsp_real values[] = {10, 2, 9, 7, 3, 8};
    brsp_size analysis_count;
    brsp_index step;
    BRSP_TRY(brsp_lu_analysis_workspace(3, &analysis_count));
    if (analysis_count > 6)
        return 1;
    BRSP_TRY(brsp_lu_plan(&pattern, analysis_work, 6, &req));
    if (req.symbolic_indices > 14 || req.factor_reals > 7 || req.numeric_work_reals > 3)
        return 1;
    BRSP_TRY(brsp_lu_analyze(&symbolic, &pattern, symbolic_storage, 14, analysis_work, 6));
    BRSP_TRY(brsp_lu_numeric_init(&numeric, &symbolic, factors, 7));
    for (step = 0; step < 4; ++step) {
        brsp_real rhs[3] = {0};
        brsp_index col;
        values[0] = 10.0F + (brsp_real)step;
        for (col = 0; col < 3; ++col) {
            brsp_index p;
            for (p = offsets[col]; p < offsets[col + 1]; ++p)
                rhs[rows[p]] += values[p] * (brsp_real)(col + 1);
        }
        BRSP_TRY(brsp_lu_refactor(&numeric, values, 6, 1.0e-7F, numeric_work, 3));
        BRSP_TRY(brsp_lu_solve(&numeric, rhs, 3, x, 3));
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
