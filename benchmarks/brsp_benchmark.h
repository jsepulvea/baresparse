#ifndef BRSP_BENCHMARK_H
#define BRSP_BENCHMARK_H
#include <baresparse/brsp.h>

#define BRSP_BENCH_N 32U
#define BRSP_BENCH_NNZ 136U
#define BRSP_BENCH_FACTORS 430U
#define BRSP_BENCH_SYMBOLIC 495U
#define BRSP_BENCH_REPETITIONS 64U
#define BRSP_BENCH_WARMUP 4U

typedef struct brsp_bench_record {
    uint32_t factor_ticks, solve_ticks;
    double backward_error, solution_error;
} brsp_bench_record;

typedef struct brsp_bench_context {
    brsp_index offsets[BRSP_BENCH_N + 1U], rows[BRSP_BENCH_NNZ];
    brsp_index analysis_work[2U * BRSP_BENCH_N];
    brsp_index symbolic_guard_before, symbolic_storage[BRSP_BENCH_SYMBOLIC], symbolic_guard_after;
    brsp_real factor_guard_before, factors[BRSP_BENCH_FACTORS], factor_guard_after;
    brsp_real work_guard_before, work[BRSP_BENCH_N], work_guard_after;
    brsp_real values[BRSP_BENCH_NNZ], rhs[BRSP_BENCH_N], x[BRSP_BENCH_N], expected[BRSP_BENCH_N];
    double error_work[2U * BRSP_BENCH_N];
    brsp_lu_symbolic symbolic;
    brsp_lu_numeric numeric;
    brsp_lu_requirements requirements;
    brsp_bench_record records[BRSP_BENCH_REPETITIONS];
    uint32_t overhead_ticks;
    volatile brsp_real observable;
} brsp_bench_context;

/* Platform transport and monotonic modulo-2^32 clock; no output in timed work. */
uint32_t brsp_bench_now(void);
void brsp_bench_write(const char *text);
void brsp_bench_uint(uintmax_t value);
void brsp_bench_metadata(const char *key, uintmax_t value);
int brsp_bench_run(brsp_bench_context *context);
#endif
