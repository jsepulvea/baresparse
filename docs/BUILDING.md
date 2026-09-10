# Building BareSparse

BareSparse uses **CMake** to describe the project and **Ninja** to execute the
build graph. The native build is the main development loop; cross-builds check
that the same core compiles for each target architecture. Hardware timing and
flashing are deliberately kept out of the portable library target.

## Prerequisites

- CMake 3.24 or newer
- Ninja
- a native C11 compiler (Apple Clang, Clang, or GCC)
- Python >= 3.9 for mutation checks and benchmark summaries

## Native development on macOS or Linux

```sh
cmake --preset host-debug
cmake --build --preset host-debug
ctest --preset host-debug
```

Run the example and development-only host benchmark:

```sh
./build/host-debug/examples/brsp_example_sparse
./build/host-debug/benchmarks/brsp_benchmark_sparse
```

For an optimized native build:

```sh
cmake --preset host-release
cmake --build --preset host-release
ctest --preset host-release
```

For memory and undefined-behavior checks:

```sh
cmake --preset host-sanitize
cmake --build --preset host-sanitize
ctest --preset host-sanitize
```

## Complete native verification and installation

```sh
./scripts/verify-host.sh
python3 scripts/check-mutations.py
```

Use `CC=gcc` or `CC=clang` on a fresh configure to choose a compiler. To keep
both builds, override the binary directory, for example:

```sh
cmake --preset host-debug -B build/clang-debug -DCMAKE_C_COMPILER=clang
cmake --build build/clang-debug
ctest --test-dir build/clang-debug --output-on-failure
```

Do not change the compiler inside an existing CMake cache. Preset sanitizer
builds instrument both the library and its test/example executables. Use a
Release install for downstream consumers:

```sh
cmake --install build/host-release --prefix "$PWD/build/install"
cmake -S tests/consumer -B build/consumer -G Ninja -DCMAKE_PREFIX_PATH="$PWD/build/install"
cmake --build build/consumer
./build/consumer/brsp_consumer
```

Consumers use `find_package(BareSparse CONFIG REQUIRED)` and link
`BareSparse::baresparse`. Compiler paths and SDK roots belong in environment
variables or ignored `CMakeUserPresets.json`. Actual tested versions and
unverified platforms are listed in [VALIDATION.md](VALIDATION.md).

## Arm Cortex-M4F cross-build

Install an Arm GNU bare-metal toolchain providing `arm-none-eabi-gcc`.
Optionally set `ARM_NONE_EABI_ROOT` to its installation prefix.

```sh
cmake --preset arm-cortex-m4-release
cmake --build --preset arm-cortex-m4-release
```

This produces the BareSparse static library for a Cortex-M4F configuration
matching the STM32G474 class of target. It does not link a board firmware image.

## TI C28x cross-build

Install the TI C2000 code-generation tools providing `cl2000` and `ar2000`.
If they are not on `PATH`, set the installation prefix:

```sh
export TI_C2000_CGT_ROOT=/path/to/ti-cgt-c2000
cmake --preset ti-c28x-release
cmake --build --preset ti-c28x-release
```

The preset builds a static C28x/EABI library using 32-bit floating-point
support. The separate `ti-f28p55x-firmware-release` preset links flash benchmark
firmware using external C2000Ware startup, linker and device support. See
[the tested compiler/SDK and board instructions](../platforms/ti/f28p55x/README.md).
Neither build preset flashes hardware.

## Microchip dsPIC33A cross-build

Install XC-DSC, which provides `xc-dsc-gcc`, `xc-dsc-ar` and `xc-dsc-ranlib`.
Current compiler releases also require an external device-family pack. Download
and extract `Microchip.dsPIC33AK-MP_DFP` from the
[Microchip pack repository](https://packs.download.microchip.com/). Set:

```sh
export XC_DSC_ROOT=/path/to/xc-dsc
export XC_DSC_DFP=/path/to/Microchip.dsPIC33AK-MP_DFP/xc16
cmake --preset microchip-dspic33a-release
cmake --build --preset microchip-dspic33a-release
```

`XC_DSC_DFP` must point to the pack's **xc16 subdirectory**, containing
`bin/device_files/33AK512MPS506.info`. It initializes the `BRSP_DSPIC_DFP` CMake
cache path, which can also be passed with `-D`. The toolchain passes `-mdfp` to
the compiler, archiver and archive indexer, and propagates it to CMake compiler
checks. Use `cmake --fresh --preset microchip-dspic33a-release` after changing
toolchain flags or recovering from a configure with a missing pack.

The checked-in preset uses `33AK512MPS506`, the Curiosity Nano reference
device. Override `BRSP_DSPIC_DEVICE` when targeting another dsPIC33A:

```sh
cmake -S . -B build/my-dspic -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/microchip-xc-dsc.cmake \
  -DBRSP_ARCH=dspic33a \
  -DBRSP_DSPIC_DEVICE=YOUR_DEVICE \
  -DBRSP_BUILD_TESTS=OFF \
  -DBRSP_BUILD_EXAMPLES=OFF \
  -DBRSP_BUILD_BENCHMARKS=OFF
cmake --build build/my-dspic
```

## Repeat both cross-build validations

With `ARM_NONE_EABI_ROOT`, `XC_DSC_ROOT` and `XC_DSC_DFP` set as above:

```sh
./scripts/verify-cross.sh
```

This performs fresh Release builds with warnings as errors, including a compile
check of the complete ordered lifecycle. It then links that consumer against
each target archive, inspects the target architecture/ABI, and checks for
unresolved strong symbols. The dsPIC link explicitly selects the DFP's device
linker script. Arm uses the toolchain's default startup and `nosys.specs`; its
warnings about absent I/O syscalls are expected for this link check. Microchip
startup's optional weak hooks may remain undefined.

The resulting `ordered-consumer.elf` files use compiler/default memory layouts.
They are link checks, **not board firmware to flash**. The script does not run
or flash them. STM32/dsPIC firmware integration and physical testing remain
separate work. Tested distribution versions and hashes are in
[VALIDATION.md](VALIDATION.md#stm32g474-and-dspic33a-compiler-evidence).

## Why cross-build only the library initially?

The numerical core should not include vendor HAL headers, startup objects,
interrupt tables, board clocks, or flash tools. Those belong to small board
firmware applications under `platforms/`. This keeps native testing fast and
makes architecture-specific kernels reusable across boards using the same ISA.

The intended development sequence is:

```text
native tests on macOS/Linux
        ↓
cross-compile the core library
        ↓
link it into board benchmark firmware
        ↓
flash and measure on physical hardware
```
