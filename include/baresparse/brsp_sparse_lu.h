#ifndef BARESPARSE_BRSP_SPARSE_LU_H
#define BARESPARSE_BRSP_SPARSE_LU_H

#include <baresparse/brsp_csc.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Experimental API. Capacities are element counts, *_bytes are C bytes.
 * See docs/SPARSE_LU.md for ownership, lifetime, aliasing and failure rules. */
typedef struct brsp_lu_ordering {
    brsp_index order;
    /* Factor index -> original index. Either NULL array means identity.
     * Each nonnull array has order entries and must be a permutation. */
    const brsp_index *row_order;
    const brsp_index *column_order;
} brsp_lu_ordering;

typedef struct brsp_lu_requirements {
    brsp_size analysis_work_indices;
    brsp_size symbolic_indices;
    brsp_size factor_reals;
    brsp_size numeric_work_reals;
    brsp_size symbolic_bytes;
    brsp_size factor_bytes;
    brsp_size analysis_work_bytes;
    brsp_size numeric_work_bytes;
    brsp_index l_nonzeros; /* Strict lower triangle; unit diagonal is implicit. */
    brsp_index u_nonzeros; /* Includes every diagonal. */
    brsp_index solve_swap_count;
} brsp_lu_requirements;

typedef struct brsp_lu_symbolic {
    brsp_csc_pattern pattern;
    const brsp_index *column_offsets;
    const brsp_index *row_indices;
    const brsp_index *diagonal_offsets;
    brsp_index factor_nonzeros;
    const brsp_index *row_order;
    const brsp_index *column_order;
    const brsp_index *inverse_row_order;
    const brsp_index *solve_swaps; /* Pairs of original vector indices. */
    brsp_index solve_swap_count;
    int ready;
} brsp_lu_symbolic;

typedef struct brsp_lu_numeric {
    const brsp_lu_symbolic *symbolic;
    brsp_real *factors;
    brsp_size capacity;
    int valid;
} brsp_lu_numeric;

/* First query: does not traverse the pattern, and needs no workspace. */
brsp_status brsp_lu_analysis_workspace(brsp_index order, brsp_size *index_count);
/* Exact storage planning with only 2*order scratch indices, no factor layout. */
brsp_status brsp_lu_plan(const brsp_csc_pattern *pattern, brsp_index *work, brsp_size work_capacity,
                         brsp_lu_requirements *requirements);
brsp_status brsp_lu_analyze(brsp_lu_symbolic *symbolic, const brsp_csc_pattern *pattern,
                            brsp_index *storage, brsp_size storage_capacity, brsp_index *work,
                            brsp_size work_capacity);
/* Ordered analysis uses 4*order scratch indices. NULL ordering selects the
 * identity API's layout and 2*order scratch requirement. Analysis copies maps
 * into symbolic storage; values, rhs and solution stay in original order. */
brsp_status brsp_lu_analysis_workspace_ordered(brsp_index order, brsp_size *index_count);
brsp_status brsp_lu_plan_ordered(const brsp_csc_pattern *pattern, const brsp_lu_ordering *ordering,
                                 brsp_index *work, brsp_size work_capacity,
                                 brsp_lu_requirements *requirements);
brsp_status brsp_lu_analyze_ordered(brsp_lu_symbolic *symbolic, const brsp_csc_pattern *pattern,
                                    const brsp_lu_ordering *ordering, brsp_index *storage,
                                    brsp_size storage_capacity, brsp_index *work,
                                    brsp_size work_capacity);
brsp_status brsp_lu_numeric_init(brsp_lu_numeric *numeric, const brsp_lu_symbolic *symbolic,
                                 brsp_real *factors, brsp_size factor_capacity);
/* Accept a fixed pivot iff finite and abs(pivot) > absolute pivot_tolerance.
 * Every call invalidates prior factors first, including on argument failure. */
brsp_status brsp_lu_refactor(brsp_lu_numeric *numeric, const brsp_real *values,
                             brsp_size value_count, brsp_real pivot_tolerance, brsp_real *work,
                             brsp_size work_capacity);
/* rhs == solution is supported. Output is unspecified on failure. */
brsp_status brsp_lu_solve(const brsp_lu_numeric *numeric, const brsp_real *rhs, brsp_size rhs_count,
                          brsp_real *solution, brsp_size solution_capacity);

#ifdef __cplusplus
}
#endif
#endif
