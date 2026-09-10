#include <baresparse/brsp.h>

#include <float.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static int brsp_test_failures = 0;

#define BRSP_TEST_CHECK(condition)                                                                 \
    do {                                                                                           \
        if (!(condition)) {                                                                        \
            (void)printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition);                      \
            ++brsp_test_failures;                                                                  \
        }                                                                                          \
    } while (0)

static brsp_real brsp_test_abs(brsp_real value) {
    return value < 0.0F ? -value : value;
}

static void brsp_test_status_strings(void) {
    BRSP_TEST_CHECK(strcmp(brsp_status_string(BRSP_STATUS_OK), "ok") == 0);
    BRSP_TEST_CHECK(strcmp(brsp_status_string(BRSP_STATUS_SINGULAR), "singular matrix") == 0);
}

static void brsp_test_csc_validation(void) {
    static const brsp_index column_offsets[] = {0U, 2U, 4U, 6U};
    static const brsp_index row_indices[] = {0U, 1U, 1U, 2U, 0U, 2U};
    static const brsp_real values[] = {10.0F, 2.0F, 9.0F, 7.0F, 3.0F, 8.0F};
    const brsp_csc_matrix matrix = {
        {3U, 3U, 6U, column_offsets, row_indices},
        values,
    };

    BRSP_TEST_CHECK(brsp_csc_matrix_validate(&matrix) == BRSP_STATUS_OK);

    {
        static const brsp_index duplicate_rows[] = {0U, 0U};
        static const brsp_index duplicate_offsets[] = {0U, 2U};
        const brsp_csc_pattern invalid = {1U, 1U, 2U, duplicate_offsets, duplicate_rows};
        BRSP_TEST_CHECK(brsp_csc_pattern_validate(&invalid) == BRSP_STATUS_INVALID_STRUCTURE);
    }
}

static void brsp_test_dense_lu(void) {
    brsp_real factor[] = {
        2.0F, 1.0F, 1.0F, 4.0F, -6.0F, 0.0F, -2.0F, 7.0F, 2.0F,
    };
    static const brsp_real right_hand_side[] = {5.0F, -2.0F, 9.0F};
    static const brsp_real expected[] = {1.0F, 1.0F, 2.0F};
    brsp_index pivots[3];
    brsp_real solution[3];
    brsp_index index;

    BRSP_TEST_CHECK(brsp_dense_lu_factor(3U, factor, 3U, pivots, 1.0e-7F) == BRSP_STATUS_OK);
    BRSP_TEST_CHECK(brsp_dense_lu_solve(3U, factor, 3U, pivots, right_hand_side, solution) ==
                    BRSP_STATUS_OK);

    for (index = 0U; index < 3U; ++index) {
        BRSP_TEST_CHECK(brsp_test_abs(solution[index] - expected[index]) < 1.0e-5F);
    }
}

static void brsp_test_singular_lu(void) {
    brsp_real factor[] = {
        1.0F,
        2.0F,
        2.0F,
        4.0F,
    };
    brsp_index pivots[2];

    BRSP_TEST_CHECK(brsp_dense_lu_factor(2U, factor, 2U, pivots, 1.0e-7F) == BRSP_STATUS_SINGULAR);
}

static void brsp_test_dense_baseline_edges(void) {
    /* Pivot swaps, padded rows, in-place RHS and padding preservation. */
    brsp_real a[] = {0, 2, 12345, 1, 3, 12345};
    brsp_real rhs[] = {4, 7};
    brsp_index pivots[2];
    BRSP_TEST_CHECK(brsp_dense_lu_factor(2, a, 3, pivots, 0) == BRSP_STATUS_OK);
    BRSP_TEST_CHECK(pivots[0] == 1 && a[2] == 12345 && a[5] == 12345);
    BRSP_TEST_CHECK(brsp_dense_lu_solve(2, a, 3, pivots, rhs, rhs) == BRSP_STATUS_OK);
    BRSP_TEST_CHECK(brsp_test_abs(rhs[0] - 1) < 1.0e-6F && brsp_test_abs(rhs[1] - 2) < 1.0e-6F);
    BRSP_TEST_CHECK(brsp_dense_lu_factor(0, NULL, 0, NULL, 0) == BRSP_STATUS_OK);
    BRSP_TEST_CHECK(brsp_dense_lu_solve(0, NULL, 0, NULL, NULL, NULL) == BRSP_STATUS_OK);
    BRSP_TEST_CHECK(brsp_dense_lu_factor(2, a, 1, pivots, 0) == BRSP_STATUS_INVALID_ARGUMENT);
    BRSP_TEST_CHECK(brsp_dense_lu_factor(2, a, 3, pivots, NAN) == BRSP_STATUS_INVALID_ARGUMENT);
    BRSP_TEST_CHECK(brsp_dense_lu_factor(2, a, 3, pivots, -1) == BRSP_STATUS_INVALID_ARGUMENT);
    a[0] = NAN;
    BRSP_TEST_CHECK(brsp_dense_lu_factor(2, a, 3, pivots, 0) == BRSP_STATUS_NONFINITE);
    a[0] = INFINITY;
    BRSP_TEST_CHECK(brsp_dense_lu_factor(2, a, 3, pivots, 0) == BRSP_STATUS_NONFINITE);
    a[0] = FLT_MIN;
    BRSP_TEST_CHECK(brsp_dense_lu_factor(1, a, 1, pivots, FLT_MIN) == BRSP_STATUS_SINGULAR);
    BRSP_TEST_CHECK(brsp_dense_lu_factor(BRSP_INDEX_MAX, a, BRSP_INDEX_MAX, pivots, 0) ==
                    BRSP_STATUS_OVERFLOW);
    BRSP_TEST_CHECK(brsp_dense_lu_solve(BRSP_INDEX_MAX, a, BRSP_INDEX_MAX, pivots, rhs, rhs) ==
                    BRSP_STATUS_OVERFLOW);
}

int main(void) {
    brsp_test_dense_baseline_edges();
    brsp_test_status_strings();
    brsp_test_csc_validation();
    brsp_test_dense_lu();
    brsp_test_singular_lu();

    if (brsp_test_failures != 0) {
        (void)printf("BareSparse: %d test failure(s)\n", brsp_test_failures);
        return 1;
    }

    (void)printf("BareSparse %s (%s): all tests passed\n", brsp_version_string(),
                 brsp_backend_name());
    return 0;
}
