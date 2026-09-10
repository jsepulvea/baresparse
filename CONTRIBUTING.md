# Contributing

Keep the public API experimental and use `brsp_` symbols / `BRSP_` constants.
Use portable C11 in the library, caller-owned typed buffers, explicit capacity
checks, and C object sizes. Never assume an eight-bit C byte. Keep vendor
headers, hardware state and timing code in `platforms/`.

Run `./scripts/verify-host.sh` before submitting changes. This exercises native
debug, release, sanitizers, examples, and installation via `find_package`.
CI runs this with GCC and Clang on Linux and Apple Clang on macOS. For numerical
changes also run `python3 scripts/check-mutations.py` and explain the tested
matrix family, precision, tolerances, factor reconstruction, backward error,
reference method and failure behavior. Tests must remain active in Release.
Default builds must retain strict floating-point semantics.

Follow `.editorconfig` and `.clang-format`: four spaces, readable C, explicit
ownership, small functions. Add tests for meaningful numerical or memory
behavior. New algorithms must not add hidden allocation or mutable global solver
state. Avoid speculative empty modules and architecture optimization without
measurements.

Hardware results must include the original raw capture, source revision, exact
compiler/flags/SDK/programmer versions, clock and timer configuration, warm-up,
repetitions, interrupt policy, code/data placement, memory units, map/code size,
numerical error for every solve, and standalone reset evidence. Clearly separate
allocated stack from measured usage and observed maximum from a WCET bound.
See [benchmark instructions](benchmarks/README.md). Do not include SDK installs,
credentials, local settings, personal paths, or generated build trees in commits.
Retain third-party notices when incorporating externally authored material.
