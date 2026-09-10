# BareSparse backlog

Physical-board tasks are deferred for the current milestone. They remain incomplete;
they are not prerequisites for the software snapshot described in [README.md](README.md).
Resume them when board access is available and hardware validation is selected
for work. No emulation is planned. Cross-compilation remains available without
boards, and existing firmware, fixtures and capture tools are retained.

## H1 — F28P55x physical execution (from M4)

- [ ] Bring up firmware on the LAUNCHXL-F28P55X with a visible pass/fail result.
      Use the existing sparse benchmark image; its compile/link validation is
      already complete.
- [ ] Run the same committed sparse fixtures on the host and board. Check target
      type widths, alignment, buffer bounds, floating-point behavior, and numerical
      acceptance criteria. Verify that the intended image runs from flash after
      reset and a power cycle.
- [ ] Record the actual board/probe configuration and probe firmware version,
      and validate the documented terminal flash/capture workflow.

**Acceptance:** the same library code tested natively runs correctly on the
F28P55x without runtime heap allocation or an operating-system dependency.
The terminal workflow builds firmware, flashes it, and retrieves a correct
sparse-solve result. Retain evidence of standalone startup.

## H2 — Reproducible hardware measurement (from M5)

The structured fixture, changing-value sequence, timing code, guards, numerical
checks and capture/summary tools already exist. Their physical behavior still
requires verification:

- [ ] Time refactorization and triangular solve separately using a hardware timer.
      Keep printing, data generation, and residual checks outside timed intervals;
      make computed outputs observable so the compiler cannot discard the work.
- [ ] Record timer frequency, wraparound handling, measurement overhead, warm-up,
      repetition count, interrupt policy, and code/data placement. An external
      logic analyzer is optional.
- [ ] Save raw results and a compact summary with compiler/version/flags, firmware
      revision, clock settings, matrix dimensions, structural nonzeros, factor
      storage, workspace, stack information, code size, and numerical error.
- [ ] Report minimum, median, and maximum observed time for the stated workload.
      Distinguish allocated stack from measured stack usage. Normalize published
      memory figures to explicitly defined octets/KiB using `CHAR_BIT` where needed.

**Acceptance:** someone with the same board can reproduce the experiment from
the committed fixture and instructions. At least one actual hardware experiment
includes numerical quality, timing, memory accounting, raw data and reproduction
instructions. Every measured solve passes the stated numerical criteria.

## H3 — Publish hardware evidence (from M6 and the original publication gates)

- [ ] Add at least one measured F28P55x result to the README and identify the
      exact configuration and source revision used, with links to the raw data.
- [ ] Update platform status to executed/benchmarked only after H1/H2 pass.
- [ ] Recheck the candidate and CI before publishing the hardware results.

The original **Embedded** and **Measured** publication gates are preserved in
H1/H2. Moving them here does not establish that they passed. Software-only
snapshots must describe hardware execution and timing as unverified.

Do not label the maximum observed time as a proven worst-case execution-time
bound. Do not claim speedups without a comparable measured baseline. Host
observations remain host results, not evidence of MCU performance.

See [F28P55x instructions](platforms/ti/f28p55x/README.md),
[benchmark procedure](benchmarks/README.md) and [validation evidence](docs/VALIDATION.md).
