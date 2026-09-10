# LAUNCHXL-F28P55X flash benchmark

**Status: cross-compiled and linked; not yet executed or benchmarked on hardware.**
The compiler and SDK were installed outside the repository for verification.
No emulator is used. Flash, reset, UART, board numerical behavior and timing
are deferred to [the physical-board backlog](../../../BACKLOG.md).
Cross-compilation does not require a connected board.

## Prerequisites and tested build

- CMake >= 3.24, Ninja, Git, Python >= 3.9 for capture/summaries.
- [TI C2000 CGT](https://www.ti.com/tool/download/C2000-CGT), tested with
  **25.11.1.LTS**, Linux x86-64 installer SHA-256 recorded in
  [validation evidence](../../../docs/VALIDATION.md).
- [C2000Ware core SDK](https://github.com/TexasInstruments/c2000ware-core-sdk),
  tested at **e5698c666d9ff587940d249213cbbb328a3bcd66**. Use the pinned
  checkout below, or supply an installed SDK and record its exact version.
- [UniFlash](https://www.ti.com/tool/UNIFLASH) with C2000/XDS110 device support
  for programming. **9.6.0.5764** was installed and its device connection
  command exercised (debug support package **21.0.0.3955**); it reports no
  connected XDS110 in this environment. Physical programming remains unverified.
  Record the probe firmware version with a hardware submission.

Install the TI compiler using its vendor installer. Keep tools outside the repo:

```sh
git clone https://github.com/TexasInstruments/c2000ware-core-sdk.git /path/to/c2000ware
git -C /path/to/c2000ware checkout e5698c666d9ff587940d249213cbbb328a3bcd66
export TI_C2000_CGT_ROOT=/path/to/ti-cgt-c2000_25.11.1.LTS
export C2000WARE_ROOT=/path/to/c2000ware
cmake --preset ti-c28x-release
cmake --build --preset ti-c28x-release
cmake --preset ti-f28p55x-firmware-release
cmake --build --preset ti-f28p55x-firmware-release
```

The first preset produces the library only. The second links
`build/ti-f28p55x-firmware-release/platforms/ti/f28p55x/brsp_f28p55x_benchmark.out`.
It also produces `brsp_f28p55x_benchmark.map` at the build root and
`brsp_f28p55x_benchmark_link.xml` in the platform build directory. Save these
with the capture. Reconfigure after committing so embedded revision metadata is
current; retain `compile_commands.json` for exact compile commands.

The CMake target selects the SDK's `f28p55x_codestartbranch.asm`,
`28p55x_generic_flash_lnk.cmd`, `device.c`, and release driverlib. These remain
external SDK files with TI's original notices. Boot entry `code_start` is at
0x080000 and branches into TI C initialization. `_FLASH` makes device startup
copy `.TI.ramfunc` to RAMLS0 before flash configuration. The numerical code
executes in flash; context is in RAMGS0, stack in RAMM1. Link flags allocate
0x400 **16-bit C bytes** (2048 octets) for stack and zero heap. That is allocated
stack capacity, **not measured usage**. The linker warning about using
`code_start` instead of `_c_int00` is expected for TI's flash startup. TI's
64-bit-double performance advice concerns residual checks outside the timed
intervals; the numerical factors remain single precision.

## Explicit flash and standalone run

Use the factory USB power/jumper arrangement, S3 GPIO24/GPIO32 = **1/1** for
flash boot, and S2 SEL1/SEL2 = **0/0** to route SCIA GPIO28/29 to the XDS110
application UART. LED5 (green, GPIO21) indicates pass; LED4 (red, GPIO20)
indicates failure. These settings follow the
[board guide, sections 2.1.1–2.1.3](https://www.ti.com/lit/ug/sprujc0a/sprujc0a.pdf).

```sh
export UNIFLASH_ROOT=/path/to/uniflash
./platforms/ti/f28p55x/flash.sh
python3 scripts/capture-serial.py /dev/ttyACM0 build/f28p55x-reset.csv
# Once the capture tool says ready, press S1 RESET on the physical board.
python3 scripts/summarize-benchmark.py build/f28p55x-reset.csv > build/f28p55x-summary.json
```

Select the **XDS110 application/user UART**, not its auxiliary data port. On
macOS supply the corresponding `/dev/cu.*` device. `BRSP_TI_CCXML` can override
the SDK's LaunchPad ccxml (e.g. to select a particular probe); keep it local.
The script invokes the Linux `DSLite` binary directly, preserving argument
boundaries (the vendor `dslite.sh` wrapper uses `eval`). For another host layout,
set `BRSP_DSLITE` to its actual CLI binary. UniFlash's installer warns about
missing legacy GUI libraries on current Ubuntu, but the headless CLI loaded
the F28P55x/XDS110 configuration and reached the missing-probe diagnostic.
The flash script uses UniFlash's documented `flash --config -f -v` operation;
see the [UniFlash command-line guide](https://software-dl.ti.com/ccs/esd/uniflash/docs/latest_qsguide.html).
Build never programs a board. The physical reset after flashing is deliberate:
a successful debugger load alone does not prove standalone flash startup.
Repeat after disconnecting/reconnecting power, open capture, and press reset
again. Retain captures from both runs and describe the board/probe configuration.

A passing run prints metadata, 64 CSV rows and `PASS` at 115200 baud, 8N1,
without flow control. `brsp_board_result` is 1 for success, 2 for failure, and
0 before completion if inspected through a debugger. If neither LED appears,
check boot/clock/power setup and UART before making numerical claims.

## Measurement contract

The [shared benchmark](../../../benchmarks/README.md) runs the same C library
and fixture as native tests. Device startup uses the SDK 20 MHz crystal PLL
configuration: SYSCLK 150 MHz, LSPCLK 37.5 MHz, 3 flash wait states. CPUTimer0
runs at SYSCLK with zero prescaler and interrupts disabled. Its down counter
is negated modulo 2^32; unsigned differences handle one wrap provided each
interval is shorter than 2^32 / 150e6 = 28.63 seconds. Do not halt the CPU
inside measured intervals; timer emulation mode is run-free.

Four verified warm-ups precede 64 verified value updates. Data generation,
residual checks, guard checks and UART output are outside both measured
intervals. Raw deltas include two clock reads/call overhead; minimum empty
interval overhead across 64 trials is recorded and is not subtracted. The
compiler cannot remove solutions: checks read them and a volatile observation
is saved. No timing maximum is a proven worst-case bound.

Hardware publication still needs a real capture, source revision, installed
UniFlash/probe versions, map/code size, and reset/power-cycle evidence. The
README must gain a hardware result only after those exist.
