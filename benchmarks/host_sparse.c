#define _POSIX_C_SOURCE 200809L
#include "brsp_benchmark.h"
#include "brsp_benchmark_build.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static brsp_bench_context brsp_host_context;

uint32_t brsp_bench_now(void) {
    struct timespec t;
    if (clock_gettime(CLOCK_MONOTONIC, &t) != 0) {
        perror("clock_gettime");
        exit(1);
    }
    return (uint32_t)((uint64_t)t.tv_sec * UINT64_C(1000000000) + (uint64_t)t.tv_nsec);
}

void brsp_bench_write(const char *text) {
    if (fputs(text, stdout) == EOF)
        exit(1);
}

int main(void) {
    brsp_bench_write("# platform=host\n# timer=POSIX CLOCK_MONOTONIC low 32 bits\n");
    brsp_bench_write("# revision=" BRSP_BENCH_REVISION "\n# compiler=" BRSP_BENCH_COMPILER "\n");
    brsp_bench_write("# flags=" BRSP_BENCH_FLAGS
                     "\n# interrupts=host OS scheduling uncontrolled\n");
    brsp_bench_write("# stack_usage=not measured\n# code_data_placement=host process\n");
    brsp_bench_metadata("timer_hz", UINT32_C(1000000000));
    return brsp_bench_run(&brsp_host_context);
}
