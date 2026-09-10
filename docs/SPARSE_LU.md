# Experimental fixed-pattern sparse LU API

`brsp_sparse_lu.h` is an **experimental API**, with no ABI stability promise.
The implementation factors square, real, canonical CSC matrices using identity
ordering by default or explicit caller-supplied row and column permutations.
`brsp_real` is a 32-bit C `float`; `brsp_index` is `uint32_t`. The dense partial-pivoting solver remains
a separate reference kernel.

## Lifecycle and storage

1. Call `brsp_lu_analysis_workspace(n, &count)` without matrix data or scratch.
   It reports exactly `2*n` scratch **index elements**, checking size overflow.
2. Supply that scratch to `brsp_lu_plan`. This validates CSC and reports exact
   symbolic, factor and numeric scratch requirements before their allocation.
3. Supply symbolic index storage and analysis scratch to `brsp_lu_analyze`.
   Analysis repeats the fill calculation and records immutable factor layouts.
4. Call `brsp_lu_numeric_init` with a completed symbolic object and a real buffer.
5. Call `brsp_lu_refactor` with new values in original CSC order, an absolute
   pivot tolerance and `n` scratch real elements. This overwrites all factors.
6. Call `brsp_lu_solve` for each right-hand side, or repeat step 5 with new values.

The [native example](../examples/host_sparse.c) exercises the entire lifecycle
using application-owned typed arrays and checks every status and requirement.
The [ordered example](../examples/host_ordered.c) uses independent row and column
maps, reuses analysis, and solves in place in original coordinates.
No library function allocates memory. There is no mutable global solver state,
recursion, variable-length array, or operating-system dependency in the core.

All capacities and counts are numbers of typed elements. Fields ending in
`_bytes` use C addressable bytes (`sizeof`), not necessarily eight-bit octets.
To report octets, multiply C bytes by `CHAR_BIT / 8.0`; divide octets by 1024
for KiB. Reports must include `CHAR_BIT`, `sizeof(brsp_real)` and
`sizeof(brsp_index)`. Do not cast an arbitrary byte buffer to these types:
use typed arrays, appropriately aligned allocations, or explicit C alignment.
The returned sizes exclude the symbolic/numeric structs and original CSC arrays.
The analysis scratch is no longer needed after analysis and may be reclaimed;
conversion to another type must respect C alignment and effective-type rules.

Initialize object structs to `{0}`. The symbolic object borrows the original
CSC arrays and the supplied symbolic buffer. The pattern struct itself is copied.
All those arrays must remain alive and unchanged while any numeric object uses
the symbolic object. Do not edit public object fields or factor layouts manually.
Analysis writes `ready=0` before work and sets it only after success. Discard all
bound numeric objects before reanalyzing or replacing their symbolic object.
Numeric initialization binds a factor buffer but does not make factors valid.
A numeric object, its factor buffer, and the symbolic object must remain alive
through solve. Values and refactor scratch are borrowed only during refactor;
rhs is borrowed only during solve. Multiple numeric objects may share one
immutable symbolic object if each has separate factor and scratch buffers.

Buffers and objects must not overlap each other, including the input CSC arrays,
values, factors and scratch. Exceptions are exact `rhs == solution` for in-place solves and sharing the
same read-only array for the two input ordering maps. Partial overlaps are
unsupported. Pointer extent, alignment,
non-overlap, and lifetime are caller preconditions; C cannot portably discover
the allocation behind a pointer. Counts must describe actual accessible arrays.

## Caller-supplied ordering

Use `brsp_lu_analysis_workspace_ordered(n, &count)`, `brsp_lu_plan_ordered`,
and `brsp_lu_analyze_ordered` for an explicit `brsp_lu_ordering`. Its `order`
must equal the matrix dimension. Each nonnull array has exactly `n` indices,
contains each index in `[0,n)` once, and maps **factor index to original index**:

```text
B[i,j] = A[row_order[i], column_order[j]]
B = L*U
ordered_rhs[i] = rhs[row_order[i]]
solution[column_order[i]] = ordered_solution[i]
```

Either null map means identity for that axis. To apply a symmetric permutation,
pass the same array for both maps. No ordering is selected automatically, and
there are no numerical pivot interchanges after analysis. A supplied row order
`{1,0}` lets `[[0,1],[1,1]]` factor successfully, whereas identity ordering
rejects its first pivot. Different values can still invalidate that fixed order.

The ordering struct and input maps are borrowed only during plan/analyze.
Analysis copies the maps into symbolic storage; the caller may then overwrite
or release them. Use the same pattern and ordering for planning and analysis to
obtain the planned sizes. Input CSC remains canonical in its **original** row
and column coordinates; do not reorder its arrays or values yourself. Numeric
initialization, refactor and solve use the existing entry points. RHS and output
also remain in original coordinates, including exact in-place solves.

Explicit ordering uses `4*n` analysis scratch indices: graph markers/stack,
inverse rows and a temporary vector permutation. In addition to the usual
factor metadata, symbolic storage contains three `n`-entry maps (rows, columns,
inverse rows) and `2*solve_swap_count` indices. Total symbolic capacity is
`5*n+1+nnz(L strict)+nnz(U)+2*solve_swap_count`. The exact swap count is reported
by plan; it is at most `n-1` for nonempty systems and zero for matching row and
column maps. Factor storage still equals predicted fill, refactor scratch still
has `n` reals, and solve needs no additional buffer. Ordering may reduce factor
fill while increasing index metadata; compare the complete byte requirements.

A null **ordering struct pointer** selects the original identity layout, with
`2*n` scratch and no stored maps. The original analysis entry points are wrappers
for that case, preserving their storage counts. An explicit ordering struct with
two null maps uses the explicit-ordering storage contract even though its order
is identity. The workspace query with the `_ordered` suffix always reports `4*n`;
use the original query for a null ordering pointer. Empty systems need one
symbolic index in either mode. Object structs have grown; rebuild consumers of
this experimental API. Historical benchmark memory results describe their
recorded source revision, not these larger structs.

## Canonical CSC and structure

Offsets have length `n+1`, start at zero, are nondecreasing, and end at `nnz`.
Rows have length `nnz`, lie in `[0,n)`, and are strictly increasing in each
column. Values have length `nnz`; refactor requires that exact count. Duplicate
or unsorted rows are rejected. Every stored entry is structural, including a
stored zero. Changing a value to zero does not change the analyzed structure.
An empty 0-by-0 system is valid: offsets still contains one zero, symbolic storage
contains one index, and all other arrays may be null with zero capacity.

## Layout and algorithms

The factor buffer is combined CSC: sorted upper entries, the U diagonal, then
strict lower entries in each column. L has an **implicit unit diagonal**. A
U pivot slot is reserved even if A has no structural diagonal. Symbolic storage
holds `n+1` column offsets, `n` diagonal offsets, then factor row indices,
followed by maps and swap pairs for explicit ordering. All factor indices refer
to the chosen elimination coordinates.
The identity storage count is `2*n+1+nnz(L strict)+nnz(U)` indices and
`nnz(L strict)+nnz(U)` reals. Numerical cancellations retain their slots.

Symbolic analysis uses the elimination graph path criterion. A factor entry
`(i,j)` exists when the matrix in elimination coordinates contains a path from
column vertex j to row vertex i with all interior vertices less than `min(i,j)`.
Traversal translates column and row indices through the stored maps while
reading the original CSC; it does not construct a reordered matrix. Explicit iterative
traversal uses `n` markers and an `n`-entry stack. It prioritizes O(n) sizing
workspace and simplicity over symbolic speed: worst-case time is
O(n²(n+nnz(A))). Analysis is outside the repeated numerical loop. Fill prediction
is for this declared fixed elimination order only.

Numerical refactorization is a sparse left-looking column algorithm. It scatters
one original column into an O(n) real work vector and applies already computed
L columns in the precomputed U row order. It does no symbolic traversal, heap
allocation, or n-by-n matrix conversion. L and U substitution traverse the same
sparse layouts, using original row positions in the solution vector as scratch.
Analysis precomputes an O(n) swap schedule that moves the solved values into
original column positions. Solve performs those swaps after substitution; it
does not discover permutation cycles during the repeated numerical loop. Clearing the numeric work
vector for each column also clears any residue from previous failed calls.

## Failure and numerical limits

- `INVALID_ARGUMENT`: null required argument, wrong shape/count, or negative or
  non-finite tolerance.
- `INVALID_STRUCTURE`: malformed offsets/rows or duplicate/out-of-range permutation indices.
- `INSUFFICIENT_CAPACITY`: a supplied buffer has fewer elements than required.
- `OVERFLOW`: order, sparse fill count, index metadata, or a C object size cannot
  be represented. Order `BRSP_INDEX_MAX` is rejected before traversing offsets.
- `NONFINITE`: non-finite values/rhs, or a non-finite intermediate/result.
- `PIVOT_REJECTED`: `abs(U[k,k]) <= pivot_tolerance` in the fixed sequence.
- `NOT_FACTORED`: solve attempted before successful refactorization or after a
  failed one. The dense reference retains its historical `SINGULAR` status.

Every refactor call with a nonnull numeric object invalidates old factors before
checking arguments. Failure can leave partial factors, which solve will refuse.
A subsequent successful refactor restores validity. Solve errors leave output
unspecified but preserve factor validity. Planning clears its requirements output
on failure. Failed analysis may alter scratch; its object is not usable.

An accepted pivot must be finite and strictly greater in magnitude than the
caller-selected **absolute** threshold. Zero threshold still rejects exact zero.
There is no equilibration, relative test, growth bound, condition estimate,
iterative refinement, or dynamic pivoting. Rescaling changes acceptance. A
nonsingular matrix such as `[[0,1],[1,1]]` is rejected under identity ordering;
pivot rejection is not a proof of singularity. Even successful factorization can be inaccurate for an
unstable fixed sequence. Callers must scale appropriately, evaluate backward
error, and select an application-specific fallback. Default builds avoid
relaxed floating-point flags such as `-ffast-math`.

## Numerical acceptance used by this repository

The committed fixtures are modest, strictly diagonally dominant single-precision
systems and small exhaustive patterns. Before interpreting output, tests set
`64*FLT_EPSILON` for normwise backward error and relative known/reference
solution error (with `1+abs(reference)` for near-zero components). Reconstruction
uses `64*FLT_EPSILON*(1+abs(A_ij))`. These are conservative for dimensions up to
32 and this well-conditioned family; they are not accuracy promises for arbitrary
matrices. The host oracle uses independent long-double Gaussian elimination
with row pivoting. Tests also reconstruct L*U and compare symbolic layouts with
independent Boolean outer-product elimination over every 3-by-3 pattern and
every 4-by-4 off-diagonal pattern. Ordered tests cover all 36 pairs of 3-entry
permutations over all 512 Boolean patterns, with numerical tests for the 64
patterns containing a dominant diagonal in elimination coordinates. They check
`L*U` against the reordered matrix and solutions/backward error against original
`A`, with additional fill-reduction, permutation-cycle and lifetime cases. The common fixture error calculation returns
zero for a zero denominator and zero residual, infinity otherwise.
