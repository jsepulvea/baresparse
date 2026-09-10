#ifndef BARESPARSE_BRSP_DENSE_LU_H
#define BARESPARSE_BRSP_DENSE_LU_H

#include <baresparse/brsp_status.h>
#include <baresparse/brsp_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * In-place row-major reference LU with partial pivoting.
 *
 * This is a dense reference kernel. The sparse fixed-pattern solver is exposed
 * separately in brsp_sparse_lu.h. The caller owns all memory.
 */
brsp_status brsp_dense_lu_factor(brsp_index order, brsp_real *matrix, brsp_index leading_dimension,
                                 brsp_index *pivots, brsp_real pivot_tolerance);

brsp_status brsp_dense_lu_solve(brsp_index order, const brsp_real *factor,
                                brsp_index leading_dimension, const brsp_index *pivots,
                                const brsp_real *right_hand_side, brsp_real *solution);

#ifdef __cplusplus
}
#endif

#endif
