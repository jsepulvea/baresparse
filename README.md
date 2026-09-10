# BareSparse

BareSparse is an allocation-free C11 library for sparse LU factorization and
linear solves on bare-metal microcontrollers.

It targets repeated systems `A_k x_k = b_k` whose numerical values change while
the sparsity pattern remains fixed. This structure occurs in Newton iterations,
implicit integration, and embedded optimization. On a memory-constrained
controller, these algorithms need factor storage and workspaces that fit the
available RAM, alongside the rest of the application's state.

BareSparse separates structural analysis from the repeated numerical solve.
Setup determines elimination fill, factor layouts, and buffer requirements.
Subsequent refactorizations reuse that structure and the same storage as matrix
values change. This keeps graph analysis and memory allocation out of the
numerical loop and allows the application to establish its solver memory budget
before entering that loop.

The implementation follows these design principles:

- **Pattern reuse:** analyze a fixed sparsity pattern once, refactor for new
  coefficients, and reuse numerical factors for multiple right-hand sides.
- **Caller-owned storage:** report explicit capacities for symbolic data,
  factors, and scratch buffers. Support static allocation and independent
  solver instances, with no heap allocation or mutable global solver state.
- **Sparse factorization:** accept compressed sparse column (CSC) input and
  store factors, including elimination fill, in sparse layouts. Numerical
  refactorization uses an O(n) workspace.
- **Explicit failure states:** use a fixed pivot sequence with optional
  caller-supplied row and column permutations. Reject unacceptable pivots and
  prevent solves from using factors left by a failed refactorization.
- **Portable core:** keep numerical code independent of operating systems,
  vendor headers, board startup, and timing instrumentation.

The reference boards considered for development and evaluation are:

| Reference board | MCU | Architecture | Current validation |
| --- | --- | --- | --- |
| TI [LAUNCHXL-F28P55X](platforms/ti/f28p55x/README.md) | F28P55x | TI C28x | Library cross-compiled; benchmark firmware linked |
| ST [NUCLEO-G474RE](platforms/st/stm32g474/README.md) | STM32G474RE | Arm Cortex-M4F | Library cross-compiled; API consumer linked |
| Microchip [dsPIC33A Curiosity Nano](platforms/microchip/dspic33a/README.md) | dsPIC33AK512MPS506 | dsPIC33A | Library cross-compiled; API consumer linked |

The API is **experimental** and currently uses 32-bit floating-point values.
Native correctness and memory checks pass on Linux and macOS. Physical-board
execution and timing measurements remain pending. Fixed pivots do not guarantee
a successful or accurate solve for every nonsingular matrix; the
[API contract](docs/SPARSE_LU.md) describes numerical limits and failure handling.

See the [build guide](docs/BUILDING.md), [working example](examples/host_sparse.c),
and [validation evidence](docs/VALIDATION.md) for usage and reproducible checks.

Licensed under [MIT](LICENSE). See [contribution instructions](CONTRIBUTING.md)
and [third-party notices](THIRD_PARTY_NOTICES.md).
