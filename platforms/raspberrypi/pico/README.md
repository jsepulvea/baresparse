# Raspberry Pi Pico RP2040 test firmware

**Status: physically flashed and passed the focused test suite over USB serial
on 2026-09-21.** See the [captured result](results/2026-09-21/README.md).

This firmware runs the same focused API suite as the native `brsp.unit` test on
an original Raspberry Pi Pico. It covers dense LU behavior, CSC validation,
status strings, edge and failure cases, and version/backend reporting. The
exhaustive sparse oracle remains a host-only test because it uses dynamic
allocation and long-double reference calculations.

## Build

The preset defaults to local copies of Pico SDK 2.3.1, its TinyUSB dependency,
host `picotool` 2.3.1, and the macOS arm64 Arm GNU Toolchain 15.3.Rel1 under the
ignored `vendor/` directory. These dependencies are not distributed with the
repository. Populate the local paths described by the preset or override
`PICO_SDK_PATH`, `PICO_TOOLCHAIN_PATH`, and `picotool_DIR` with compatible
installations before building:

```sh
cmake --preset pico-rp2040-tests-release
cmake --build --preset pico-rp2040-tests-release
```

The flash image is:

```text
build/pico-rp2040-tests-release/platforms/raspberrypi/pico/brsp_pico_tests.uf2
```

The same directory also contains ELF, BIN, HEX, `.elf.map`, and disassembly
outputs. The reference compiler is host-specific, so another host architecture
requires compatible toolchain and picotool installations.

## Manual flash and test

1. Disconnect the Pico, hold **BOOTSEL**, reconnect USB, and release **BOOTSEL**.
2. Copy `brsp_pico_tests.uf2` to the mounted `RPI-RP2` volume.
3. Open the newly enumerated USB CDC serial device. On macOS it is normally a
   `/dev/cu.usbmodem*` device; the baud-rate selection is ignored by USB CDC.

The onboard LED blinks every 500 ms while the firmware waits for a USB serial
connection. It then runs the tests once. A passing run prints:

```text
BareSparse 0.1.0 (portable): all tests passed
```

Afterward, a steady LED means pass. A rapid 125 ms blink means at least one
assertion failed; the USB log contains each failing source location and
condition. The firmware deliberately remains in the final indication loop.

Building never detects or programs a board automatically. The 2026-09-21 run
verified BOOTSEL flashing, firmware boot, USB enumeration, and the test result;
the LED indication and a separate power-cycle run remain unverified.
