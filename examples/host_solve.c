#include <baresparse/brsp.h>

#include <stdio.h>

int main(void)
{
    brsp_real factor[] = {
        2.0F, 1.0F, 1.0F,
        4.0F, -6.0F, 0.0F,
        -2.0F, 7.0F, 2.0F,
    };
    static const brsp_real right_hand_side[] = {5.0F, -2.0F, 9.0F};
    brsp_index pivots[3];
    brsp_real solution[3];
    brsp_status status;

    status = brsp_dense_lu_factor(3U, factor, 3U, pivots, 1.0e-7F);
    if (status == BRSP_STATUS_OK) {
        status = brsp_dense_lu_solve(
            3U, factor, 3U, pivots, right_hand_side, solution);
    }

    if (status != BRSP_STATUS_OK) {
        (void)fprintf(stderr, "solve failed: %s\n", brsp_status_string(status));
        return 1;
    }

    (void)printf("BareSparse %s, backend %s\n", brsp_version_string(), brsp_backend_name());
    (void)printf("x = [%.6f, %.6f, %.6f]\n",
                 (double)solution[0],
                 (double)solution[1],
                 (double)solution[2]);
    return 0;
}
