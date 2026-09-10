#ifndef BRSP_STRUCTURED_FIXTURE_H
#define BRSP_STRUCTURED_FIXTURE_H

#include <baresparse/brsp.h>
#include <float.h>
#include <math.h>

/* Original structured synthetic fixture: 4 by 8 nonsymmetric five-point grid.
 * Column-major CSC with row-major grid numbering. See benchmarks/README.md. */
#define BRSP_FIXTURE_N 32U
#define BRSP_FIXTURE_NNZ 136U
#define BRSP_FIXTURE_STEPS 64U
#define BRSP_FIXTURE_TOLERANCE (64.0 * FLT_EPSILON)

static void brsp_fixture_pattern(brsp_index *offsets, brsp_index *rows) {
    brsp_index col;
    brsp_index p = 0U;
    offsets[0] = 0U;
    for (col = 0U; col < BRSP_FIXTURE_N; ++col) {
        brsp_index row;
        for (row = 0U; row < BRSP_FIXTURE_N; ++row) {
            if (row == col || row + 8U == col || col + 8U == row ||
                (row / 8U == col / 8U && (row + 1U == col || col + 1U == row))) {
                rows[p++] = row;
            }
        }
        offsets[col + 1U] = p;
    }
}

static void brsp_fixture_values(const brsp_index *offsets, const brsp_index *rows, brsp_index step,
                                brsp_real *values, brsp_real *rhs, brsp_real *expected) {
    brsp_index col;
    for (col = 0U; col < BRSP_FIXTURE_N; ++col) {
        expected[col] = 1.0F + (brsp_real)col / 32.0F;
        rhs[col] = 0.0F;
    }
    for (col = 0U; col < BRSP_FIXTURE_N; ++col) {
        brsp_index p;
        for (p = offsets[col]; p < offsets[col + 1U]; ++p) {
            const brsp_index row = rows[p];
            values[p] = row == col ? 6.0F + (brsp_real)(step % BRSP_FIXTURE_STEPS) / 128.0F
                                   : (row < col ? -0.75F : -1.0F);
            rhs[row] += values[p] * expected[col];
        }
    }
}

/* Shared host/board check. Scratch: 2*n doubles. Returns infinity on nonfinite
 * data, with explicit eta=0 for the all-zero denominator/residual case. */
static double brsp_fixture_error(const brsp_csc_pattern *a, const brsp_real *values,
                                 const brsp_real *rhs, const brsp_real *x, double *work) {
    brsp_index i;
    double anorm = 0.0, bnorm = 0.0, xnorm = 0.0, residual = 0.0;
    for (i = 0U; i < a->row_count; ++i) {
        if (!isfinite(rhs[i]) || !isfinite(x[i])) {
            return INFINITY;
        }
        work[i] = rhs[i];
        work[a->row_count + i] = 0.0;
        if (fabs((double)rhs[i]) > bnorm)
            bnorm = fabs((double)rhs[i]);
        if (fabs((double)x[i]) > xnorm)
            xnorm = fabs((double)x[i]);
    }
    for (i = 0U; i < a->column_count; ++i) {
        brsp_index p;
        for (p = a->column_offsets[i]; p < a->column_offsets[i + 1U]; ++p) {
            const brsp_index row = a->row_indices[p];
            if (!isfinite(values[p]))
                return INFINITY;
            work[row] -= (double)values[p] * x[i];
            work[a->row_count + row] += fabs((double)values[p]);
        }
    }
    for (i = 0U; i < a->row_count; ++i) {
        if (fabs(work[i]) > residual)
            residual = fabs(work[i]);
        if (work[a->row_count + i] > anorm)
            anorm = work[a->row_count + i];
    }
    if (!isfinite(residual) || !isfinite(anorm * xnorm + bnorm))
        return INFINITY;
    return anorm * xnorm + bnorm == 0.0 ? (residual == 0.0 ? 0.0 : INFINITY)
                                        : residual / (anorm * xnorm + bnorm);
}
#endif
