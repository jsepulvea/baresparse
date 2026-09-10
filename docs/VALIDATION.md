# Validation evidence and remaining gates

Validation date: **2026-09-10**. This is a development candidate, not a completed
public-release claim. Tests establish behavior for their stated matrix families,
not general numerical robustness of fixed pivots.

## Native evidence

Linux x86-64 tools used locally:

| Tool | Actual version |
| --- | --- |
| GCC | Ubuntu 15.2.0-16ubuntu1 |
| Clang | Ubuntu 21.1.8 |
| CMake | 4.2.3 |
| Ninja | 1.13.2 |

All `host-debug`, `host-release`, and `host-sanitize` configurations pass with
GCC and Clang, with project warnings treated as errors. The suite runs the dense
baseline, sparse numerical/memory/API checks, identity and ordered sparse
examples, and the shared host benchmark. `scripts/verify-host.sh` additionally installs Release
and builds/runs a separate consumer through `find_package(BareSparse)` and
`BareSparse::baresparse`. Use a fresh build directory for each compiler.

Coverage includes 512 possible 3-by-3 patterns and all 4096 off-diagonal patterns
of a 4-by-4 system with dominant diagonals, plus a 32-variable nonsymmetric grid.
These include diagonal, triangular, banded, disconnected and fill-producing
patterns. Symbolic output is compared with an independent Boolean elimination
oracle. Numeric output is checked against known solutions and a separately
implemented long-double, partial-pivoting Gaussian solver, and by reconstructing
L*U. The 64-step grid sequence analyzes once and validates every solve.

Caller-supplied ordering adds 18,432 symbolic cases: every 3-by-3 pattern under
all 36 row/column permutation pairs. The 2,304 cases with dominant diagonals in
elimination coordinates additionally check reconstructed reordered matrices,
known/reference solutions in original coordinates, in-place solves and backward
error. Other cases cover row-only/column-only/symmetric/identity maps, absent
structural diagonals, reduced fill (16 to 10 stored factors), disjoint and longer
permutation cycles, exact/short storage, invalid maps, destroyed caller maps,
empty systems and recovery after a rejected pivot. An installed consumer also
runs the new ordering example through four value updates.

Failure tests cover CSC offsets/rows, null arguments, wrong counts, nonsquare
shape, exactly sized/undersized buffers, empty systems, index/object-size
boundaries, nonfinite values/tolerance/rhs, generated nonfinite arithmetic, zero
and threshold pivots, an invertible matrix requiring row interchange, later
pivot failure, repeated recovery, implicit/explicit zeros, in-place RHS and
independent numeric instances. Guard checks and ASan/UBSan cover memory behavior.
`python3 scripts/check-mutations.py` verifies the suite rejects deliberate
indexing, ignored permutations, incorrect output permutation, reported-workspace-size,
and backward-substitution defects; each
mutation compiles before the test failure is accepted as evidence.

Runtime source audit: no allocation, recursion, VLA, OS calls, or mutable global
state in `src/brsp_sparse_lu.c`. Refactorization traverses only planned layouts;
its real scratch is O(n). Solve uses the output vector as scratch. Native archive
undefined-symbol inspection contains only internal CSC validation, compiler
stack-check support and `memset`, with no allocator symbols. No source is built
with `-ffast-math`; TI uses explicit `--fp_mode=strict`.

The [recorded host benchmark](../benchmarks/results/host-2026-09-10/README.md)
uses the implemented ordering code with the shared identity-order fixture. All
64 measured solves passed. Its raw capture retains the pre-consolidation revision;
a committed SHA-256 manifest identifies the exact build inputs in the fresh
snapshot. The summary script reproduces the committed JSON from the raw CSV.

The CI workflow runs the complete native verification script on Linux GCC,
Linux Clang and macOS Apple Clang, with all five mutation checks on Linux GCC.
The verified tool matrix is GCC 13.3.0, Clang 18.1.3 and Apple Clang
21.0.0.21000101. Each job runs five tests in debug, release and sanitizer builds,
then runs both installed-package examples. Cross-compiler checks are local
checks described below; they are not jobs in this native CI workflow.

The repository uses a single root commit on `main`, titled
`First commit fresh history`. Check the
[main branch CI results](https://github.com/jsepulvea/baresparse/actions?query=branch%3Amain)
for that commit before changing visibility. Earlier development commits are
not part of this branch's history. The repository remains private for the owner
to publish; physical-board tasks stay in backlog.

## TI compiler and firmware evidence

TI C2000 CGT **25.11.1.LTS** was installed outside the source tree from TI's
Linux x86-64 distribution. Installer SHA-256:

```text
2f3a978e24a53279c9796ad374c32b530fc58fd085fd71288597846fe169c821
```

C2000Ware core SDK:

```text
release: REL_C2000Ware_v26.01.00.00.STS
revision: e5698c666d9ff587940d249213cbbb328a3bcd66
```

Both `ti-c28x-release` and `ti-f28p55x-firmware-release` configure and build.
The latter links the same portable sparse sources with SDK startup, device
initialization and release driverlib. Link map entry is `code_start` at
0x080000. The benchmark context is placed in RAMGS0; `.TI.ramfunc` loads from
flash and runs in RAMLS0; stack allocation is 0x400 C bytes in RAMM1. Heap size
is zero and the map has no `malloc`, `calloc`, `realloc`, or `free` symbols.
Target static assertions verify 16-bit C bytes and 32-bit real/index storage;
alignments and memory counts are also emitted by the firmware at runtime.

Flags: C11, EABI, silicon version 28, large memory model, FPU32, strict floating
point, function subsections, optimization level 2. The SDK firmware translation
units use relaxed C11 to permit TI peripheral intrinsics; the library remains
strict C11. Keep the actual compile commands and link map with hardware results.
64-bit double advice from TI concerns untimed residual checks. The nonstandard
entry-point warning is expected for TI's flash startup.

**Not verified:** physical programming, standalone reset or power-cycle boot,
UART output, LED behavior, target numerical quality, actual clock/timer behavior,
measured stack usage, or board timing. `lsusb` shows no TI/XDS110 device and
there is no serial device for a board. UniFlash **9.6.0.5764** is installed with debug support package
**21.0.0.3955**. A read-only `DSLite flash --config=... --list-cores --timeout=10`
request loads the configuration but returns TI error **-260**, no XDS110
connection. Probe firmware therefore remains unrecorded. Installer SHA-256:
`66e78bfa083492999c524a6ea97c16b4b90be54d65beae7701210fb6404b0d9c`.
Missing legacy GUI libraries were reported by the installer; they did not
prevent this headless device-connection diagnostic. No emulation has been used.
Physical execution and measurement acceptance are preserved as backlog H1/H2;
they remain unverified and do not block current software work. See the
[reproduction instructions](../platforms/ti/f28p55x/README.md).

## STM32G474 and dsPIC33A compiler evidence

Both libraries and compile-only ordered API consumers were built on Linux
x86-64 with their actual cross-compilers using `scripts/verify-cross.sh`.
Release C11 builds (`-O3 -DNDEBUG`) pass with all project warnings treated as
errors. The script also links each API consumer against its target archive and
vendor runtime; neither has unresolved strong symbols. These are compiler/link
checks only: no STM32 or dsPIC board firmware port or execution is claimed.

| Target | Distribution | Compiler identity | Target evidence |
| --- | --- | --- | --- |
| STM32G474-compatible Cortex-M4F | Arm GNU Toolchain 15.2.Rel1 | arm-none-eabi-gcc 15.2.1 20251203 | ELF32 Arm, v7E-M, Thumb-2, VFPv4-D16, single-precision hardware FP, VFP register arguments |
| dsPIC33AK512MPS506 | Microchip XC-DSC 4.00.00 | GCC 8.3.1, build Jun 24 2026, `__XC_DSC_VERSION__=40000` | Vendor objdump reports elf32-pic30 and architecture 33AK512MPS506 |

The [Arm distribution](https://developer.arm.com/-/media/Files/downloads/gnu/15.2.rel1/binrel/arm-gnu-toolchain-15.2.rel1-x86_64-arm-none-eabi.tar.xz),
[XC-DSC distribution](https://ww1.microchip.com/downloads/aemDocuments/documents/DEV/ProductDocuments/SoftwareTools/xc-dsc-v4.00-full-install-linux64.tar.xz)
and [dsPIC33AK-MP DFP 1.6.273](https://packs.download.microchip.com/Microchip.dsPIC33AK-MP_DFP.1.6.273.atpack)
were downloaded and extracted outside the repository. Distribution SHA-256:

```text
Arm 15.2.Rel1 x86_64 arm-none-eabi tar.xz
597893282ac8c6ab1a4073977f2362990184599643b4c5ee34870a8215783a16
XC-DSC 4.00 Linux tar.xz
4fa70007bf28e246c39ec280270f510a8e003993671297c412ccf8e57c8eacef
dsPIC33AK-MP DFP 1.6.273 atpack
7e45f7f536b91af6f8e4d4e16d97f539e5cf8c7f0dd89e0aa2bfa4fcb499cae4
```

Arm flags include `-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard`.
The existing Arm toolchain needed no portability changes. dsPIC uses
`-mcpu=33AK512MPS506 -mdfp=<pack>/xc16`. Both use function/data sections and
retain strict floating-point behavior. The device pack is required by current
XC-DSC releases, as described in the
[vendor release notes](https://ww1.microchip.com/downloads/aemDocuments/documents/DEV/ProductDocuments/UserGuides/Readme_XC-DSC.html).
Without it, the compiler rejected the CPU name. The dsPIC CMake toolchain now
accepts `XC_DSC_DFP` / `BRSP_DSPIC_DFP`, validates the device metadata path,
propagates it through compiler checks, and passes the pack to the compiler,
archiver and required vendor ranlib. This also removes the archive tools'
missing-resource diagnostics. No solver-source workaround was necessary.

The dsPIC consumer link explicitly selects `p33AK512MPS506.gld`; the compiler's
default linker layout alone did not select the correct output architecture.
Arm links with its default runtime and `nosys.specs`, whose unimplemented I/O
stub warnings are expected. Microchip startup leaves four optional weak hooks
undefined. The script checks strong references, including the complete ordered
lifecycle. The generated ELF files are not board firmware to flash.
See [reproduction commands](BUILDING.md#repeat-both-cross-build-validations).

The updated ordering sources also rebuild with TI CGT 25.11.1.LTS and link the
F28P55x benchmark firmware. Identity factor/symbolic buffer requirements remain
unchanged, while public object structs include the ordering metadata. The
recorded host capture accounts for those structs. Physical tasks stay in backlog.

## Publication audit

| Validation area | Evidence / remaining requirement |
| --- | --- |
| M0 baseline | Native matrix, clean checkout, examples and installation pass locally and in Linux/macOS CI |
| M1 contract | Public header, SPARSE_LU.md and lifecycle example implemented and tested |
| M2 sparse pipeline | Exact symbolic fill, sparse numeric storage, reuse and substitution verified |
| M3 correctness | Independent references, reconstruction, quality, failures, guards, sanitizers and mutation detection pass |
| M4 software preparation | Actual TI compile/link passes; physical validation moved to backlog H1 |
| M5 infrastructure/host evidence | Shared fixture, runner, capture/summary tools and real host results complete; board experiment moved to backlog H2 |
| M6 preparation/publication | One root commit on main contains the software snapshot; CI and source review prepare it for the owner to make public |

## History and attribution review

The fresh snapshot retains all implemented library code, examples, tests,
platform tooling and MIT attribution. Its single root commit consolidates the
private development history as requested by the owner. A recovery bundle of the
previous history is kept outside the repository and is not pushed.

The publication tree was reviewed for generated/vendor paths, binary content,
local compiler paths, and common private-key/GitHub-token/AWS-key signatures.
No matches were found. This is a bounded review, not a guarantee from a secret
scanner. `LICENSE` retains the original copyright; `THIRD_PARTY_NOTICES.md`
describes external compiler, SDK and runtime material without vendoring it.
The host source manifest and raw-to-summary comparison preserve reproducibility
without requiring the discarded development history.

The implemented software scope is described in [README.md](../README.md).
[BACKLOG.md](../BACKLOG.md) preserves physical-board acceptance, timing, raw-data
and README hardware-result tasks. Deferring them does not establish completion
or permit hardware performance claims. The owner will make the prepared remote
public after checking the final `main` commit and CI.
