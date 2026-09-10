#include <baresparse/brsp.h>

#include <stdint.h>
#include <stdio.h>
#include <time.h>

#define BRSP_BENCHMARK_ITERATIONS 100000U

int main(void)
{
    static const brsp_real original[] = {
        4.0F, 1.0F, 0.0F, 1.0F,
        1.0F, 5.0F, 1.0F, 0.0F,
        0.0F, 1.0F, 6.0F, 1.0F,
        1.0F, 0.0F, 1.0F, 7.0F,
    };
    static const brsp_real right_hand_side[] = {1.0F, 2.0F, 3.0F, 4.0F};
    brsp_real factor[16];
    brsp_real solution[4];
    brsp_index pivots[4];
    uint32_t iteration;
    clock_t start;
    clock_t finish;
    double elapsed_seconds;

    start = clock();
    for (iteration = 0U; iteration < BRSP_BENCHMARK_ITERATIONS; ++iteration) {
        brsp_index index;
        for (index = 0U; index < 16U; ++index) {
            factor[index] = original[index];
        }

        if (brsp_dense_lu_factor(4U, factor, 4U, pivots, 1.0e-7F) != BRSP_STATUS_OK) {
            return 1;
        }
        if (brsp_dense_lu_solve(
                4U, factor, 4U, pivots, right_hand_side, solution) != BRSP_STATUS_OK) {
            return 1;
        }
    }
    finish = clock();

    elapsed_seconds = (double)(finish - start) / (double)CLOCKS_PER_SEC;
    (void)printf("backend: %s\n", brsp_backend_name());
    (void)printf("iterations: %u\n", (unsigned int)BRSP_BENCHMARK_ITERATIONS);
    (void)printf("elapsed: %.6f s\n", elapsed_seconds);
    (void)printf("last x[0]: %.6f\n", (double)solution[0]);
    (void)printf("Host timing is for development only, not an MCU benchmark.\n");
    return 0;
}
