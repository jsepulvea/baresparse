# Raspberry Pi Pico physical test — 2026-09-21

The focused BareSparse test firmware was flashed and executed successfully on
an original Raspberry Pi Pico.

## Configuration

- Board bootloader: UF2 v3.0, model `Raspberry Pi RP2`, board ID `RPI-RP2`.
- Runtime USB identity: Raspberry Pi Pico, VID:PID `2e8a:000a`, serial
  `E66480454F76A732`.
- Host: macOS 26.6.2 (25G83), arm64.
- Source base revision: `c306a2a2d0b5d5faf472f9784a56de6e5c698693`, plus the
  Pico support changes committed alongside this record.
- Compiler: Arm GNU Toolchain 15.3.Rel1, `arm-none-eabi-gcc 15.3.1 20260627`.
- SDK: Raspberry Pi Pico SDK 2.3.1 with TinyUSB at
  `86ad6e56c1700e85f1c5678607a762cfe3aa2f47`.

## Image and execution

The board was connected in BOOTSEL mode and identified through
`INFO_UF2.TXT`. The following image was copied to the `RPI-RP2` volume:

```text
UF2 SHA-256: b2cd76ba1a18789e58a71f19c16eb0f8d6a6ba37294c02c1327efb9f98737b15
ELF SHA-256: 0f955fa8e44b1d0c8c5f25d1b3624da7287493e9de0361df082adb9dcc44f042
```

The boot volume detached, the firmware enumerated as
`/dev/cu.usbmodem21201`, and opening that USB CDC endpoint triggered the test
run. The complete captured output is in [serial.txt](serial.txt):

```text
BareSparse 0.1.0 (portable): all tests passed
```

The linked ELF is ARM EABI5 for RP2040/Cortex-M0+, with 41,204 bytes of text
and 3,288 bytes of BSS as reported by `arm-none-eabi-size`. This is functional
test evidence, not a performance measurement. The LED indication and a separate
power-cycle run were not independently observed.
