#include "brsp_benchmark.h"
#include "fixtures/structured.h"

void brsp_bench_uint(uintmax_t value) {
    char digits[sizeof(uintmax_t) * CHAR_BIT + 1U];
    brsp_size pos = sizeof(digits) / sizeof(digits[0]) - 1U;
    digits[pos] = '\0';
    do {
        digits[--pos] = (char)('0' + value % 10U);
        value /= 10U;
    } while (value > 0U);
    brsp_bench_write(digits + pos);
}

void brsp_bench_metadata(const char *key, uintmax_t value) {
    brsp_bench_write("# ");
    brsp_bench_write(key);
    brsp_bench_write("=");
    brsp_bench_uint(value);
    brsp_bench_write("\n");
}

static int brsp_bench_status(brsp_status status) {
    if (status == BRSP_STATUS_OK)
        return 1;
    brsp_bench_write("FAIL: ");
    brsp_bench_write(brsp_status_string(status));
    brsp_bench_write("\n");
    return 0;
}

int brsp_bench_run(brsp_bench_context *c) {
    brsp_csc_pattern a = {BRSP_BENCH_N, BRSP_BENCH_N, BRSP_BENCH_NNZ, c->offsets, c->rows};
    brsp_index step;
    uint32_t start;
    c->symbolic_guard_before = c->symbolic_guard_after = BRSP_INDEX_MAX;
    c->factor_guard_before = c->factor_guard_after = 12345.0F;
    c->work_guard_before = c->work_guard_after = 12345.0F;
    brsp_fixture_pattern(c->offsets, c->rows);
    if (!brsp_bench_status(brsp_lu_plan(&a, c->analysis_work, 2U * BRSP_BENCH_N, &c->requirements)))
        return 1;
    if (c->requirements.symbolic_indices != BRSP_BENCH_SYMBOLIC ||
        c->requirements.factor_reals != BRSP_BENCH_FACTORS) {
        brsp_bench_write("FAIL: fixture storage changed\n");
        return 1;
    }
    if (!brsp_bench_status(brsp_lu_analyze(&c->symbolic, &a, c->symbolic_storage,
                                           BRSP_BENCH_SYMBOLIC, c->analysis_work,
                                           2U * BRSP_BENCH_N)))
        return 1;
    if (!brsp_bench_status(
            brsp_lu_numeric_init(&c->numeric, &c->symbolic, c->factors, BRSP_BENCH_FACTORS)))
        return 1;
    c->overhead_ticks = UINT32_MAX;
    for (step = 0; step < 64U; ++step) {
        uint32_t elapsed;
        start = brsp_bench_now();
        elapsed = brsp_bench_now() - start;
        if (elapsed < c->overhead_ticks)
            c->overhead_ticks = elapsed;
    }
    for (step = 0; step < BRSP_BENCH_WARMUP + BRSP_BENCH_REPETITIONS; ++step) {
        const brsp_index sample = step < BRSP_BENCH_WARMUP ? 0U : step - BRSP_BENCH_WARMUP;
        brsp_status factor_status, solve_status;
        brsp_bench_record record;
        brsp_index i;
        brsp_fixture_values(c->offsets, c->rows, sample, c->values, c->rhs, c->expected);
        start = brsp_bench_now();
        factor_status = brsp_lu_refactor(&c->numeric, c->values, BRSP_BENCH_NNZ, 1.0e-7F, c->work,
                                         BRSP_BENCH_N);
        record.factor_ticks = brsp_bench_now() - start;
        if (!brsp_bench_status(factor_status))
            return 1;
        start = brsp_bench_now();
        solve_status = brsp_lu_solve(&c->numeric, c->rhs, BRSP_BENCH_N, c->x, BRSP_BENCH_N);
        record.solve_ticks = brsp_bench_now() - start;
        if (!brsp_bench_status(solve_status))
            return 1;
        c->observable = c->x[sample % BRSP_BENCH_N];
        record.backward_error = brsp_fixture_error(&a, c->values, c->rhs, c->x, c->error_work);
        record.solution_error = 0.0;
        for (i = 0; i < BRSP_BENCH_N; ++i) {
            const double error = fabs((double)c->x[i] - c->expected[i]) / c->expected[i];
            if (error > record.solution_error)
                record.solution_error = error;
        }
        if (!isfinite(record.backward_error) || record.backward_error > BRSP_FIXTURE_TOLERANCE ||
            record.solution_error > BRSP_FIXTURE_TOLERANCE ||
            c->symbolic_guard_before != BRSP_INDEX_MAX ||
            c->symbolic_guard_after != BRSP_INDEX_MAX || c->factor_guard_before != 12345.0F ||
            c->factor_guard_after != 12345.0F || c->work_guard_before != 12345.0F ||
            c->work_guard_after != 12345.0F) {
            brsp_bench_write("FAIL: numerical quality or buffer guard\n");
            return 1;
        }
        if (step >= BRSP_BENCH_WARMUP)
            c->records[sample] = record;
    }
    brsp_bench_metadata("char_bit", CHAR_BIT);
    brsp_bench_metadata("real_c_bytes", sizeof(brsp_real));
    brsp_bench_metadata("index_c_bytes", sizeof(brsp_index));
    brsp_bench_metadata("real_alignment_c_bytes", _Alignof(brsp_real));
    brsp_bench_metadata("index_alignment_c_bytes", _Alignof(brsp_index));
    brsp_bench_metadata("n", BRSP_BENCH_N);
    brsp_bench_metadata("nnz_a", BRSP_BENCH_NNZ);
    brsp_bench_metadata("nnz_l_strict", c->requirements.l_nonzeros);
    brsp_bench_metadata("nnz_u", c->requirements.u_nonzeros);
    brsp_bench_metadata("symbolic_c_bytes", c->requirements.symbolic_bytes);
    brsp_bench_metadata("factor_c_bytes", c->requirements.factor_bytes);
    brsp_bench_metadata("analysis_work_c_bytes", c->requirements.analysis_work_bytes);
    brsp_bench_metadata("numeric_work_c_bytes", c->requirements.numeric_work_bytes);
    brsp_bench_metadata("benchmark_context_c_bytes", sizeof(*c));
    brsp_bench_metadata("warmup", BRSP_BENCH_WARMUP);
    brsp_bench_metadata("repetitions", BRSP_BENCH_REPETITIONS);
    brsp_bench_metadata("timer_overhead_ticks_min", c->overhead_ticks);
    brsp_bench_write("# overhead_subtracted=false\n# error_units=1e-12 (rounded upward)\n");
    brsp_bench_write("sample,factor_ticks,solve_ticks,backward_error_e12,solution_error_e12\n");
    for (step = 0; step < BRSP_BENCH_REPETITIONS; ++step) {
        const brsp_bench_record *r = &c->records[step];
        brsp_bench_uint(step);
        brsp_bench_write(",");
        brsp_bench_uint(r->factor_ticks);
        brsp_bench_write(",");
        brsp_bench_uint(r->solve_ticks);
        brsp_bench_write(",");
        brsp_bench_uint((uintmax_t)ceil(r->backward_error * 1.0e12));
        brsp_bench_write(",");
        brsp_bench_uint((uintmax_t)ceil(r->solution_error * 1.0e12));
        brsp_bench_write("\n");
    }
    brsp_bench_write("PASS\n");
    return 0;
}
