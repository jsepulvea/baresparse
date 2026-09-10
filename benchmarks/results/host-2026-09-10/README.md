# Linux host observation, 2026-09-10

**Host result, not an MCU benchmark.** This capture uses the implemented
caller-supplied-ordering code, with identity ordering for the shared fixture. Raw data is [raw.csv](raw.csv);
[summarize-benchmark.py](../../../scripts/summarize-benchmark.py) reproduces
[summary.json](summary.json). All 64 measured solves passed numerical checks.

| Configuration | Value |
| --- | --- |
| Capture revision | `a939a61ced8d235308fc6afa6cbc0895c23899b7` (clean checkout before history consolidation) |
| Source identity after consolidation | [SHA-256 input manifest](source-manifest.sha256), matching the sources in the fresh snapshot |
| Host CPU | 12th Gen Intel Core i7-12700H, x86-64 |
| OS/kernel | Linux 7.0.0-22-generic |
| Compiler | GCC Ubuntu 15.2.0-16ubuntu1 |
| Preset / numerical flags | `host-release`, C11, `-O3 -DNDEBUG`; no fast math or LTO |
| CMake / Ninja | 4.2.3 / 1.13.2 |
| Timer | CLOCK_MONOTONIC, 1e9 ticks/s, uint32 modulo delta |
| Warm-up / repetitions | 4 / 64 |
| Minimum empty interval | 20 ns, not subtracted |
| Interrupt/scheduling policy | Host OS controlled; no affinity or isolation |
| Code / data placement | Normal ELF process; no special placement |
| Stack | OS-provided, allocated limit not recorded, usage not measured |

| Operation | Minimum (µs) | Median (µs) | Maximum observed (µs) |
| --- | ---: | ---: | ---: |
| Refactorization | 2.188 | 2.197 | 2.304 |
| Triangular solve | 0.596 | 0.598 | 0.628 |

Maximum backward error <= 1.16201e-7; maximum relative known-solution error
<= 3.17892e-7 (raw errors rounded upward at 1e-12). Preselected tolerance is
64*FLT_EPSILON = 7.62939453125e-6. Matrix dimensions are 32x32, nnz(A)=136,
strict nnz(L)=199 and nnz(U)=231. L's unit diagonal is implicit.

Storage in **eight-bit octets** (`CHAR_BIT=8` here): symbolic 1980, numeric
factors 1720, analysis scratch 256, numerical scratch 128. Full benchmark context
is 7984 octets including those arrays, input, checks, guards and records. Do not
add context size to its component arrays. GNU `size` for this exact executable
reported text=17665, data=640 and BSS=8016 octets; these include the host runner
and reporting code and are not embedded code sizes.

The capture retains its original pre-consolidation revision as provenance; that
commit is not part of the fresh public history. The manifest identifies the
exact library, fixture, runner, headers and build scripts present in this snapshot,
so reproduction does not require retrieving an earlier commit. Verify those
inputs from the repository root, then run the host commands in
[benchmarks/README.md](../../README.md):

```sh
sha256sum -c benchmarks/results/host-2026-09-10/source-manifest.sha256
```

On macOS use `shasum -a 256 -c` with the same manifest. A rebuilt runner prints
the new checkout's Git revision; this changes provenance text, not solver input.
 CPU power state, contention and scheduling
will change observations; no speedup or worst-case execution-time bound is claimed.
