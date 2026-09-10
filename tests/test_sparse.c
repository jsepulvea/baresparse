#include "../benchmarks/fixtures/structured.h"
#include <baresparse/brsp.h>

#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BRSP_CHECK(c)                                                                              \
    do {                                                                                           \
        if (!(c)) {                                                                                \
            (void)fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #c);                          \
            exit(1);                                                                               \
        }                                                                                          \
    } while (0)
#define BRSP_OK(c) BRSP_CHECK((c) == BRSP_STATUS_OK)
#define BRSP_TEST_MAX_N 32U
#define BRSP_TEST_TOL (64.0L * FLT_EPSILON)

/* Independent host oracle: long-double Gaussian elimination with row pivoting,
 * applied directly to augmented [A|b], rather than reuse of library factors. */
static void brsp_reference(brsp_index n, const long double *a, const brsp_real *b, long double *x) {
    long double augmented[BRSP_TEST_MAX_N][BRSP_TEST_MAX_N + 1U];
    brsp_index i, j, k;
    for (i = 0U; i < n; ++i) {
        for (j = 0U; j < n; ++j)
            augmented[i][j] = a[i * n + j];
        augmented[i][n] = b[i];
    }
    for (k = 0U; k < n; ++k) {
        brsp_index pivot = k;
        for (i = k + 1U; i < n; ++i)
            if (fabsl(augmented[i][k]) > fabsl(augmented[pivot][k]))
                pivot = i;
        BRSP_CHECK(fabsl(augmented[pivot][k]) > LDBL_MIN);
        for (j = k; j <= n; ++j) {
            const long double t = augmented[k][j];
            augmented[k][j] = augmented[pivot][j];
            augmented[pivot][j] = t;
        }
        for (i = k + 1U; i < n; ++i) {
            const long double multiplier = augmented[i][k] / augmented[k][k];
            for (j = k; j <= n; ++j)
                augmented[i][j] -= multiplier * augmented[k][j];
        }
    }
    for (i = n; i > 0U; --i) {
        const brsp_index row = i - 1U;
        x[row] = augmented[row][n];
        for (j = row + 1U; j < n; ++j)
            x[row] -= augmented[row][j] * x[j];
        x[row] /= augmented[row][row];
    }
}

static void brsp_check_ordered_case(const brsp_csc_pattern *a, brsp_real *values,
                                    const brsp_lu_ordering *ordering, int numeric_test) {
    const brsp_index n = a->column_count;
    brsp_lu_requirements req;
    brsp_lu_symbolic symbolic = {0};
    brsp_lu_numeric numeric = {0};
    brsp_size nw;
    brsp_index *iw, *storage;
    brsp_real *factor, *work;
    long double dense[BRSP_TEST_MAX_N * BRSP_TEST_MAX_N] = {0};
    long double lower[BRSP_TEST_MAX_N * BRSP_TEST_MAX_N] = {0};
    long double upper[BRSP_TEST_MAX_N * BRSP_TEST_MAX_N] = {0};
    unsigned char original[BRSP_TEST_MAX_N * BRSP_TEST_MAX_N] = {0};
    unsigned char fill[BRSP_TEST_MAX_N * BRSP_TEST_MAX_N] = {0};
    brsp_index row_order[BRSP_TEST_MAX_N], column_order[BRSP_TEST_MAX_N];
    brsp_real rhs[BRSP_TEST_MAX_N], x[BRSP_TEST_MAX_N];
    long double reference[BRSP_TEST_MAX_N];
    double error_work[2U * BRSP_TEST_MAX_N];
    brsp_index i, j, k, p;
    brsp_size count = 0U;
    for (i = 0U; i < n; ++i) {
        row_order[i] = ordering != NULL && ordering->row_order != NULL ? ordering->row_order[i] : i;
        column_order[i] =
            ordering != NULL && ordering->column_order != NULL ? ordering->column_order[i] : i;
    }
    BRSP_OK(ordering == NULL ? brsp_lu_analysis_workspace(n, &nw)
                             : brsp_lu_analysis_workspace_ordered(n, &nw));
    iw = malloc((nw + 2U) * sizeof(*iw));
    BRSP_CHECK(iw != NULL);
    iw[0] = iw[nw + 1U] = BRSP_INDEX_MAX;
    BRSP_OK(brsp_lu_plan_ordered(a, ordering, iw + 1, nw, &req));
    BRSP_CHECK(req.analysis_work_indices == nw);
    BRSP_CHECK(req.symbolic_bytes == req.symbolic_indices * sizeof(brsp_index));
    BRSP_CHECK(req.factor_bytes == req.factor_reals * sizeof(brsp_real));
    BRSP_CHECK(req.numeric_work_reals == n);
    storage = malloc((req.symbolic_indices + 2U) * sizeof(*storage));
    factor = malloc((req.factor_reals + 2U) * sizeof(*factor));
    work = malloc(((brsp_size)n + 2U) * sizeof(*work));
    BRSP_CHECK(storage != NULL && factor != NULL && work != NULL);
    storage[0] = storage[req.symbolic_indices + 1U] = BRSP_INDEX_MAX;
    factor[0] = factor[req.factor_reals + 1U] = 12345.0F;
    work[0] = work[n + 1U] = 12345.0F;
    if (ordering != NULL) {
        brsp_index input_rows[BRSP_TEST_MAX_N], input_columns[BRSP_TEST_MAX_N];
        brsp_lu_ordering temporary = {n, ordering->row_order == NULL ? NULL : input_rows,
                                      ordering->column_order == NULL ? NULL : input_columns};
        for (i = 0U; i < n; ++i) {
            input_rows[i] = row_order[i];
            input_columns[i] = column_order[i];
        }
        BRSP_OK(brsp_lu_analyze_ordered(&symbolic, a, &temporary, storage + 1, req.symbolic_indices,
                                        iw + 1, nw));
        /* Destroy caller maps before numerical work; analysis must own copies. */
        for (i = 0U; i < n; ++i) {
            input_rows[i] = input_columns[i] = BRSP_INDEX_MAX;
            BRSP_CHECK(symbolic.row_order != input_rows && symbolic.column_order != input_columns);
            BRSP_CHECK(symbolic.row_order[i] == row_order[i]);
            BRSP_CHECK(symbolic.column_order[i] == column_order[i]);
        }
    } else {
        BRSP_OK(brsp_lu_analyze(&symbolic, a, storage + 1, req.symbolic_indices, iw + 1, nw));
    }
    for (j = 0U; j < n; ++j) {
        for (p = a->column_offsets[j]; p < a->column_offsets[j + 1U]; ++p) {
            original[a->row_indices[p] * n + j] = 1U;
            dense[a->row_indices[p] * n + j] = values[p];
        }
    }
    for (i = 0U; i < n; ++i)
        for (j = 0U; j < n; ++j)
            fill[i * n + j] =
                (unsigned char)(i == j || original[row_order[i] * n + column_order[j]]);
    /* Independent symbolic oracle: explicit Boolean outer-product elimination. */
    for (k = 0U; k < n; ++k)
        for (i = k + 1U; i < n; ++i)
            for (j = k + 1U; j < n; ++j)
                if (fill[i * n + k] && fill[k * n + j])
                    fill[i * n + j] = 1U;
    for (j = 0U; j < n; ++j) {
        BRSP_CHECK(symbolic.column_offsets[j] == count);
        for (i = 0U; i < n; ++i) {
            if (fill[i * n + j]) {
                BRSP_CHECK(count < req.factor_reals);
                BRSP_CHECK(symbolic.row_indices[count] == i);
                if (i == j)
                    BRSP_CHECK(symbolic.diagonal_offsets[j] == count);
                ++count;
            }
        }
    }
    BRSP_CHECK(count == req.factor_reals);
    BRSP_CHECK(count == (brsp_size)req.l_nonzeros + req.u_nonzeros);
    BRSP_CHECK(symbolic.column_offsets[n] == count);
    if (numeric_test) {
        BRSP_OK(brsp_lu_numeric_init(&numeric, &symbolic, factor + 1, req.factor_reals));
        BRSP_OK(brsp_lu_refactor(&numeric, values, a->nonzero_count, 0.0F, work + 1, n));
        for (j = 0U; j < n; ++j) {
            lower[j * n + j] = 1.0L;
            for (p = symbolic.column_offsets[j]; p < symbolic.column_offsets[j + 1U]; ++p) {
                i = symbolic.row_indices[p];
                if (i > j)
                    lower[i * n + j] = numeric.factors[p];
                else
                    upper[i * n + j] = numeric.factors[p];
            }
        }
        for (i = 0U; i < n; ++i) {
            for (j = 0U; j < n; ++j) {
                long double reconstructed = 0.0L;
                for (k = 0U; k < n; ++k)
                    reconstructed += lower[i * n + k] * upper[k * n + j];
                BRSP_CHECK(fabsl(reconstructed - dense[row_order[i] * n + column_order[j]]) <=
                           BRSP_TEST_TOL *
                               (1.0L + fabsl(dense[row_order[i] * n + column_order[j]])));
            }
        }
        for (k = 0U; k < 3U; ++k) {
            for (i = 0U; i < n; ++i) {
                long double b = 0.0L;
                for (j = 0U; j < n; ++j)
                    b += dense[i * n + j] * (long double)(j + k + 1U);
                rhs[i] = (brsp_real)b;
                x[i] = rhs[i];
            }
            brsp_reference(n, dense, rhs, reference);
            BRSP_OK(brsp_lu_solve(&numeric, k == 1U ? x : rhs, n, x, n));
            for (i = 0U; i < n; ++i) {
                BRSP_CHECK(fabsl((long double)x[i] - reference[i]) <=
                           BRSP_TEST_TOL * (1.0L + fabsl(reference[i])));
                BRSP_CHECK(fabsl((long double)x[i] - (long double)(i + k + 1U)) <=
                           BRSP_TEST_TOL * (long double)(i + k + 1U));
            }
            BRSP_CHECK(brsp_fixture_error(a, values, rhs, x, error_work) <= BRSP_FIXTURE_TOLERANCE);
        }
    }
    BRSP_CHECK(iw[0] == BRSP_INDEX_MAX && iw[nw + 1U] == BRSP_INDEX_MAX);
    BRSP_CHECK(storage[0] == BRSP_INDEX_MAX &&
               storage[req.symbolic_indices + 1U] == BRSP_INDEX_MAX);
    BRSP_CHECK(factor[0] == 12345.0F && factor[req.factor_reals + 1U] == 12345.0F);
    BRSP_CHECK(work[0] == 12345.0F && work[n + 1U] == 12345.0F);
    free(iw);
    free(storage);
    free(factor);
    free(work);
}

static void brsp_check_case(const brsp_csc_pattern *a, brsp_real *values, int numeric_test) {
    brsp_check_ordered_case(a, values, NULL, numeric_test);
}

/* Every 3x3 Boolean pattern in factor coordinates, with all 36 pairs of row
 * and column permutations. Build original CSC independently from the maps. */
static void brsp_ordered_patterns(void) {
    static const brsp_index permutations[6][3] = {{0, 1, 2}, {0, 2, 1}, {1, 0, 2},
                                                  {1, 2, 0}, {2, 0, 1}, {2, 1, 0}};
    unsigned r, c, mask;
    for (r = 0U; r < 6U; ++r) {
        for (c = 0U; c < 6U; ++c) {
            const brsp_lu_ordering ordering = {3U, permutations[r], permutations[c]};
            for (mask = 0U; mask < 512U; ++mask) {
                unsigned char present[9] = {0};
                brsp_real dense[9] = {0}, values[9];
                brsp_index offsets[4], rows[9], i, j, p = 0U;
                brsp_csc_pattern a = {3U, 3U, 0U, offsets, rows};
                for (i = 0U; i < 3U; ++i) {
                    for (j = 0U; j < 3U; ++j) {
                        const brsp_index position = permutations[r][i] * 3U + permutations[c][j];
                        present[position] = (unsigned char)((mask >> (i * 3U + j)) & 1U);
                        dense[position] = i == j ? 4.0F : -0.25F;
                    }
                }
                offsets[0] = 0U;
                for (j = 0U; j < 3U; ++j) {
                    for (i = 0U; i < 3U; ++i) {
                        if (present[i * 3U + j]) {
                            rows[p] = i;
                            values[p++] = dense[i * 3U + j];
                        }
                    }
                    offsets[j + 1U] = p;
                }
                a.nonzero_count = p;
                brsp_check_ordered_case(&a, values, &ordering, (mask & 273U) == 273U);
            }
        }
    }
}

static void brsp_ordering_contract(void) {
    brsp_index offsets[] = {0, 2, 4}, rows[] = {0, 1, 0, 1};
    brsp_csc_pattern a = {2, 2, 4, offsets, rows};
    brsp_index row_order[] = {1, 0}, column_order[] = {0, 1};
    brsp_lu_ordering ordering = {2, row_order, column_order};
    brsp_lu_symbolic s = {0};
    brsp_lu_numeric numeric = {0};
    brsp_lu_requirements req;
    brsp_index iw[8], storage[17];
    brsp_real values[] = {0, 1, 1, 1}, factor[4], work[2], rhs[] = {2, 3}, x[2];
    brsp_size count;
    unsigned step;
    BRSP_CHECK(brsp_lu_analysis_workspace_ordered(2, NULL) == BRSP_STATUS_INVALID_ARGUMENT);
    BRSP_CHECK(brsp_lu_analysis_workspace_ordered(BRSP_INDEX_MAX, &count) == BRSP_STATUS_OVERFLOW);
    BRSP_CHECK(count == 0);
    BRSP_OK(brsp_lu_analysis_workspace_ordered(2, &count));
    BRSP_CHECK(count == 8);
    BRSP_CHECK(brsp_lu_plan_ordered(&a, &ordering, iw, 7, &req) ==
               BRSP_STATUS_INSUFFICIENT_CAPACITY);
    BRSP_CHECK(req.symbolic_indices == 0 && req.solve_swap_count == 0);
    BRSP_CHECK(brsp_lu_plan_ordered(&a, &ordering, NULL, 8, &req) == BRSP_STATUS_INVALID_ARGUMENT);
    BRSP_CHECK(brsp_lu_plan_ordered(&a, &ordering, iw, 8, NULL) == BRSP_STATUS_INVALID_ARGUMENT);
    ordering.order = 1;
    BRSP_CHECK(brsp_lu_plan_ordered(&a, &ordering, iw, 8, &req) == BRSP_STATUS_INVALID_ARGUMENT);
    ordering.order = 2;
    row_order[0] = 0;
    BRSP_CHECK(brsp_lu_plan_ordered(&a, &ordering, iw, 8, &req) == BRSP_STATUS_INVALID_STRUCTURE);
    row_order[0] = 2;
    BRSP_CHECK(brsp_lu_plan_ordered(&a, &ordering, iw, 8, &req) == BRSP_STATUS_INVALID_STRUCTURE);
    row_order[0] = 1;
    column_order[0] = 1;
    BRSP_CHECK(brsp_lu_plan_ordered(&a, &ordering, iw, 8, &req) == BRSP_STATUS_INVALID_STRUCTURE);
    column_order[0] = BRSP_INDEX_MAX;
    BRSP_CHECK(brsp_lu_plan_ordered(&a, &ordering, iw, 8, &req) == BRSP_STATUS_INVALID_STRUCTURE);
    column_order[0] = 0;
    BRSP_OK(brsp_lu_plan_ordered(&a, &ordering, iw, 8, &req));
    BRSP_CHECK(req.symbolic_indices == 17 && req.factor_reals == 4 && req.solve_swap_count == 1);
    BRSP_CHECK(brsp_lu_analyze_ordered(&s, &a, &ordering, storage, 16, iw, 8) ==
               BRSP_STATUS_INSUFFICIENT_CAPACITY);
    BRSP_CHECK(!s.ready);
    BRSP_CHECK(brsp_lu_analyze_ordered(&s, &a, &ordering, NULL, 17, iw, 8) ==
               BRSP_STATUS_INVALID_ARGUMENT);
    BRSP_CHECK(brsp_lu_analyze_ordered(NULL, &a, &ordering, storage, 17, iw, 8) ==
               BRSP_STATUS_INVALID_ARGUMENT);
    BRSP_OK(brsp_lu_analyze_ordered(&s, &a, &ordering, storage, 17, iw, 8));
    row_order[0] = 0;
    BRSP_CHECK(brsp_lu_analyze_ordered(&s, &a, &ordering, storage, 17, iw, 8) ==
               BRSP_STATUS_INVALID_STRUCTURE);
    BRSP_CHECK(!s.ready);
    row_order[0] = 1;
    BRSP_OK(brsp_lu_analyze_ordered(&s, &a, &ordering, storage, 17, iw, 8));
    row_order[0] = BRSP_INDEX_MAX;
    column_order[0] = BRSP_INDEX_MAX;
    BRSP_OK(brsp_lu_numeric_init(&numeric, &s, factor, 4));
    for (step = 0; step < 3; ++step) {
        BRSP_OK(brsp_lu_refactor(&numeric, values, 4, 0, work, 2));
        BRSP_OK(brsp_lu_solve(&numeric, rhs, 2, x, 2));
        BRSP_CHECK(x[0] == 1 && x[1] == 2);
        values[1] = 0; /* Chosen first pivot fails; the analysis is reusable. */
        BRSP_CHECK(brsp_lu_refactor(&numeric, values, 4, 0, work, 2) == BRSP_STATUS_PIVOT_REJECTED);
        BRSP_CHECK(brsp_lu_solve(&numeric, rhs, 2, x, 2) == BRSP_STATUS_NOT_FACTORED);
        values[1] = 1;
    }
    /* Old entry points still reserve exactly the original identity storage. */
    BRSP_OK(brsp_lu_analyze(&s, &a, storage, 9, iw, 4));
    BRSP_CHECK(s.row_order == NULL && s.column_order == NULL && s.solve_swap_count == 0);
    BRSP_OK(brsp_lu_numeric_init(&numeric, &s, factor, 4));
    BRSP_CHECK(brsp_lu_refactor(&numeric, values, 4, 0, work, 2) == BRSP_STATUS_PIVOT_REJECTED);
    row_order[0] = 1;
    ordering.column_order = NULL;
    brsp_check_ordered_case(&a, values, &ordering, 1);
    ordering.row_order = NULL;
    ordering.column_order = row_order;
    brsp_check_ordered_case(&a, values, &ordering, 1);
    ordering.column_order = NULL;
    values[0] = 4;
    brsp_check_ordered_case(&a, values, &ordering, 1);
    a.row_count = a.column_count = a.nonzero_count = ordering.order = 0;
    a.row_indices = NULL;
    BRSP_OK(brsp_lu_plan_ordered(&a, &ordering, NULL, 0, &req));
    BRSP_CHECK(req.symbolic_indices == 1 && req.analysis_work_indices == 0);
    BRSP_OK(brsp_lu_analyze_ordered(&s, &a, &ordering, storage, 1, NULL, 0));
    BRSP_OK(brsp_lu_numeric_init(&numeric, &s, NULL, 0));
    BRSP_OK(brsp_lu_refactor(&numeric, NULL, 0, 0, NULL, 0));
    BRSP_OK(brsp_lu_solve(&numeric, NULL, 0, NULL, 0));
}

static void brsp_ordering_fill_and_cycles(void) {
    brsp_index offsets[] = {0, 4, 6, 8, 10}, rows[] = {0, 1, 2, 3, 0, 1, 0, 2, 0, 3};
    brsp_real values[] = {4, -1, -1, -1, -1, 4, -1, 4, -1, 4};
    const brsp_index leaves_first[] = {1, 2, 3, 0};
    brsp_lu_ordering ordering = {4, leaves_first, leaves_first};
    brsp_csc_pattern a = {4, 4, 10, offsets, rows};
    brsp_index iw[20];
    brsp_lu_requirements req;
    BRSP_OK(brsp_lu_plan(&a, iw, 8, &req));
    BRSP_CHECK(req.factor_reals == 16);
    BRSP_OK(brsp_lu_plan_ordered(&a, &ordering, iw, 16, &req));
    BRSP_CHECK(req.factor_reals == 10 && req.solve_swap_count == 0);
    brsp_check_ordered_case(&a, values, &ordering, 1);
    {
        /* One 3-cycle plus one 2-cycle, then a single 5-cycle. */
        const brsp_index cycles[2][5] = {{1, 2, 0, 4, 3}, {1, 2, 3, 4, 0}};
        brsp_index diagonal_offsets[] = {0, 1, 2, 3, 4, 5};
        brsp_index permuted_rows[5];
        brsp_real diagonal_values[] = {2, 3, 4, 5, 6};
        brsp_csc_pattern diagonal = {5, 5, 5, diagonal_offsets, permuted_rows};
        unsigned cycle;
        for (cycle = 0; cycle < 2; ++cycle) {
            brsp_index i;
            brsp_lu_ordering row_only = {5, cycles[cycle], NULL};
            for (i = 0; i < 5; ++i)
                permuted_rows[i] = cycles[cycle][i];
            BRSP_OK(brsp_lu_plan_ordered(&diagonal, &row_only, iw, 20, &req));
            BRSP_CHECK(req.solve_swap_count == (cycle == 0 ? 3U : 4U));
            brsp_check_ordered_case(&diagonal, diagonal_values, &row_only, 1);
        }
    }
}

static void brsp_exhaustive_patterns(void) {
    unsigned mask;
    for (mask = 0U; mask < 512U; ++mask) {
        brsp_index offsets[4], rows[9], i, j, p = 0U;
        brsp_real values[9];
        brsp_csc_pattern a = {3U, 3U, 0U, offsets, rows};
        offsets[0] = 0U;
        for (j = 0U; j < 3U; ++j) {
            for (i = 0U; i < 3U; ++i) {
                if ((mask & (1U << (i * 3U + j))) != 0U) {
                    rows[p] = i;
                    values[p++] = i == j ? 4.0F : -0.25F;
                }
            }
            offsets[j + 1U] = p;
        }
        a.nonzero_count = p;
        brsp_check_case(&a, values, (mask & 273U) == 273U);
    }
    /* All 4x4 off-diagonal patterns, with dominant diagonals. */
    for (mask = 0U; mask < 4096U; ++mask) {
        brsp_index offsets[5], rows[16], i, j, p = 0U, bit = 0U;
        brsp_real values[16];
        brsp_csc_pattern a = {4U, 4U, 0U, offsets, rows};
        offsets[0] = 0U;
        for (j = 0U; j < 4U; ++j) {
            for (i = 0U; i < 4U; ++i) {
                const int present = i == j || (mask & (1U << bit)) != 0U;
                if (i != j)
                    ++bit;
                if (present) {
                    rows[p] = i;
                    values[p++] = i == j ? 4.0F : -0.5F;
                }
            }
            offsets[j + 1U] = p;
        }
        a.nonzero_count = p;
        brsp_check_case(&a, values, 1);
    }
}

static void brsp_failures(void) {
    brsp_index offsets[] = {0U, 2U, 4U}, rows[] = {0U, 1U, 0U, 1U};
    brsp_csc_pattern a = {2U, 2U, 4U, offsets, rows};
    brsp_lu_requirements req;
    brsp_lu_symbolic s = {0};
    brsp_lu_numeric num = {0}, other = {0};
    brsp_index iw[4], storage[9];
    brsp_real factors[4], other_factors[4], w[2], values[] = {2, 1, 1, 3};
    brsp_real rhs[] = {3, 4}, x[2];
    brsp_size count;
    unsigned repeat;
    BRSP_CHECK(brsp_lu_analysis_workspace(BRSP_INDEX_MAX, &count) == BRSP_STATUS_OVERFLOW);
    BRSP_CHECK(brsp_lu_analysis_workspace(2, NULL) == BRSP_STATUS_INVALID_ARGUMENT);
    BRSP_CHECK(brsp_lu_plan(NULL, iw, 4, &req) == BRSP_STATUS_INVALID_ARGUMENT);
    BRSP_CHECK(brsp_lu_plan(&a, iw, 3, &req) == BRSP_STATUS_INSUFFICIENT_CAPACITY);
    BRSP_CHECK(brsp_lu_plan(&a, NULL, 4, &req) == BRSP_STATUS_INVALID_ARGUMENT);
    BRSP_CHECK(brsp_lu_analyze(&s, &a, storage, 8, iw, 4) == BRSP_STATUS_INSUFFICIENT_CAPACITY);
    BRSP_CHECK(!s.ready);
    BRSP_OK(brsp_lu_analyze(&s, &a, storage, 9, iw, 4));
    BRSP_CHECK(brsp_lu_numeric_init(&num, &s, factors, 3) == BRSP_STATUS_INSUFFICIENT_CAPACITY);
    BRSP_OK(brsp_lu_numeric_init(&num, &s, factors, 4));
    BRSP_OK(brsp_lu_numeric_init(&other, &s, other_factors, 4));
    BRSP_CHECK(brsp_lu_solve(&num, rhs, 2, x, 2) == BRSP_STATUS_NOT_FACTORED);
    BRSP_OK(brsp_lu_refactor(&other, values, 4, 0, w, 2));
    for (repeat = 0U; repeat < 3U; ++repeat) {
        BRSP_OK(brsp_lu_refactor(&num, values, 4, 0, w, 2));
        BRSP_CHECK(brsp_lu_refactor(&num, values, 4, 0, w, 1) == BRSP_STATUS_INSUFFICIENT_CAPACITY);
        BRSP_CHECK(brsp_lu_solve(&num, rhs, 2, x, 2) == BRSP_STATUS_NOT_FACTORED);
        BRSP_CHECK(brsp_lu_refactor(&num, values, 3, 0, w, 2) == BRSP_STATUS_INVALID_ARGUMENT);
        BRSP_CHECK(brsp_lu_refactor(&num, values, 4, NAN, w, 2) == BRSP_STATUS_INVALID_ARGUMENT);
        BRSP_CHECK(brsp_lu_refactor(&num, values, 4, -1, w, 2) == BRSP_STATUS_INVALID_ARGUMENT);
        BRSP_CHECK(brsp_lu_refactor(&num, values, 4, INFINITY, w, 2) ==
                   BRSP_STATUS_INVALID_ARGUMENT);
        values[0] = NAN;
        BRSP_CHECK(brsp_lu_refactor(&num, values, 4, 0, w, 2) == BRSP_STATUS_NONFINITE);
        values[0] = INFINITY;
        BRSP_CHECK(brsp_lu_refactor(&num, values, 4, 0, w, 2) == BRSP_STATUS_NONFINITE);
        values[0] = 0; /* Nonsingular, but needs a row interchange. */
        BRSP_CHECK(brsp_lu_refactor(&num, values, 4, 0, w, 2) == BRSP_STATUS_PIVOT_REJECTED);
        values[0] = 1.0e-8F;
        BRSP_CHECK(brsp_lu_refactor(&num, values, 4, 1.0e-8F, w, 2) == BRSP_STATUS_PIVOT_REJECTED);
        values[0] = FLT_MIN;
        values[1] = FLT_MAX;
        BRSP_CHECK(brsp_lu_refactor(&num, values, 4, 0, w, 2) == BRSP_STATUS_NONFINITE);
        values[0] = 2;
        values[1] = 1;
        values[3] = 0.5F; /* Later pivot failure. */
        BRSP_CHECK(brsp_lu_refactor(&num, values, 4, 0, w, 2) == BRSP_STATUS_PIVOT_REJECTED);
        BRSP_CHECK(brsp_lu_solve(&num, rhs, 2, x, 2) == BRSP_STATUS_NOT_FACTORED);
        BRSP_OK(brsp_lu_solve(&other, rhs, 2, x, 2));
        BRSP_CHECK(fabsf(x[0] - 1) < 1.0e-6F && fabsf(x[1] - 1) < 1.0e-6F);
        values[3] = 3;
    }
    values[1] = 0; /* Explicit zero retains the analyzed structural entry. */
    BRSP_OK(brsp_lu_refactor(&num, values, 4, 0, w, 2));
    BRSP_CHECK(s.factor_nonzeros == 4);
    BRSP_OK(brsp_lu_solve(&num, rhs, 2, x, 2));
    BRSP_CHECK(brsp_lu_solve(&num, rhs, 2, x, 1) == BRSP_STATUS_INSUFFICIENT_CAPACITY);
    BRSP_CHECK(brsp_lu_solve(&num, rhs, 1, x, 2) == BRSP_STATUS_INVALID_ARGUMENT);
    rhs[0] = NAN;
    BRSP_CHECK(brsp_lu_solve(&num, rhs, 2, x, 2) == BRSP_STATUS_NONFINITE);
    rows[1] = 0;
    BRSP_CHECK(brsp_lu_plan(&a, iw, 4, &req) == BRSP_STATUS_INVALID_STRUCTURE);
    rows[0] = 1;
    rows[1] = 0;
    BRSP_CHECK(brsp_lu_plan(&a, iw, 4, &req) == BRSP_STATUS_INVALID_STRUCTURE);
    rows[0] = 0;
    rows[1] = 2;
    BRSP_CHECK(brsp_lu_plan(&a, iw, 4, &req) == BRSP_STATUS_INVALID_STRUCTURE);
    rows[1] = 1;
    offsets[0] = 1;
    BRSP_CHECK(brsp_lu_plan(&a, iw, 4, &req) == BRSP_STATUS_INVALID_STRUCTURE);
    offsets[0] = 0;
    offsets[1] = 5;
    BRSP_CHECK(brsp_lu_plan(&a, iw, 4, &req) == BRSP_STATUS_INVALID_STRUCTURE);
    offsets[1] = 2;
    offsets[2] = 3;
    BRSP_CHECK(brsp_lu_plan(&a, iw, 4, &req) == BRSP_STATUS_INVALID_STRUCTURE);
    a.column_count = BRSP_INDEX_MAX;
    a.row_count = BRSP_INDEX_MAX;
    BRSP_CHECK(brsp_lu_plan(&a, iw, 4, &req) == BRSP_STATUS_OVERFLOW);
    BRSP_CHECK(brsp_csc_pattern_validate(&a) == BRSP_STATUS_OVERFLOW);
    a.column_count = 2;
    a.row_count = 1;
    BRSP_CHECK(brsp_lu_plan(&a, iw, 4, &req) == BRSP_STATUS_INVALID_ARGUMENT);
    a.column_count = a.row_count = a.nonzero_count = 0;
    a.row_indices = NULL;
    BRSP_OK(brsp_lu_plan(&a, NULL, 0, &req));
    BRSP_CHECK(req.symbolic_indices == 1 && req.factor_reals == 0);
    BRSP_OK(brsp_lu_analyze(&s, &a, storage, 1, NULL, 0));
    BRSP_OK(brsp_lu_numeric_init(&num, &s, NULL, 0));
    BRSP_OK(brsp_lu_refactor(&num, NULL, 0, 0, NULL, 0));
    BRSP_OK(brsp_lu_solve(&num, NULL, 0, NULL, 0));
}

static void brsp_sequence(void) {
    brsp_index offsets[BRSP_FIXTURE_N + 1U], rows[BRSP_FIXTURE_NNZ];
    brsp_index iw[2U * BRSP_FIXTURE_N], storage[1024];
    brsp_real values[BRSP_FIXTURE_NNZ], rhs[BRSP_FIXTURE_N], expected[BRSP_FIXTURE_N];
    brsp_real factors[1024], work[BRSP_FIXTURE_N], x[BRSP_FIXTURE_N];
    double ew[2U * BRSP_FIXTURE_N];
    brsp_csc_pattern a = {BRSP_FIXTURE_N, BRSP_FIXTURE_N, BRSP_FIXTURE_NNZ, offsets, rows};
    brsp_lu_symbolic s = {0};
    brsp_lu_numeric num = {0};
    brsp_lu_requirements req;
    brsp_index step, i;
    brsp_fixture_pattern(offsets, rows);
    BRSP_OK(brsp_lu_plan(&a, iw, 2U * BRSP_FIXTURE_N, &req));
    BRSP_CHECK(req.factor_reals < BRSP_FIXTURE_N * BRSP_FIXTURE_N / 2U);
    BRSP_OK(brsp_lu_analyze(&s, &a, storage, 1024, iw, 2U * BRSP_FIXTURE_N));
    BRSP_OK(brsp_lu_numeric_init(&num, &s, factors, 1024));
    for (step = 0U; step < BRSP_FIXTURE_STEPS; ++step) {
        brsp_fixture_values(offsets, rows, step, values, rhs, expected);
        BRSP_OK(brsp_lu_refactor(&num, values, BRSP_FIXTURE_NNZ, 1.0e-7F, work, BRSP_FIXTURE_N));
        BRSP_OK(brsp_lu_solve(&num, rhs, BRSP_FIXTURE_N, x, BRSP_FIXTURE_N));
        for (i = 0; i < BRSP_FIXTURE_N; ++i)
            BRSP_CHECK(fabs((double)x[i] - expected[i]) <= BRSP_FIXTURE_TOLERANCE * expected[i]);
        BRSP_CHECK(brsp_fixture_error(&a, values, rhs, x, ew) <= BRSP_FIXTURE_TOLERANCE);
        if (step == 0 || step == BRSP_FIXTURE_STEPS - 1U)
            brsp_check_case(&a, values, 1);
    }
    memset(values, 0, sizeof(values));
    memset(rhs, 0, sizeof(rhs));
    memset(x, 0, sizeof(x));
    BRSP_CHECK(brsp_fixture_error(&a, values, rhs, x, ew) == 0.0);
}

int main(void) {
    brsp_exhaustive_patterns();
    brsp_ordered_patterns();
    brsp_ordering_contract();
    brsp_ordering_fill_and_cycles();
    brsp_failures();
    brsp_sequence();
    (void)puts(
        "Sparse LU: exhaustive fill, reconstruction, reference, lifecycle and guards passed");
    return 0;
}
