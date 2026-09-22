# Raspberry Pi Pico development cheatsheet

This document records the RP2040 workflow exercised on macOS for BareSparse.
Commands are run from the repository root unless stated otherwise.

## Local layout

The following directories are local development inputs and are ignored by Git:

```text
vendor/                         Pico SDK, toolchain, picotool, and sources
scratch/pico_blink/             Independent blink and USB-reset project
scratch/ram_layout/             Independent RAM-layout reporting project
```

Relevant local tools and dependencies:

```text
vendor/pico-sdk/
vendor/toolchain/expanded/Payload/bin/arm-none-eabi-*
vendor/picotool-src/
scratch/pico_blink/build-picotool-usb/picotool
```

`vendor/` is not repository content and must not be treated as BareSparse MIT
source. The project documentation describes these packages as external build
dependencies.

## Independent blink project

Build the RP2040 blink firmware:

```sh
./scratch/pico_blink/build.sh
```

Principal output:

```text
scratch/pico_blink/build/pico_blink.uf2
```

The program initializes USB stdio and toggles GPIO 25, the original Pico's
onboard LED. Its CMake configuration enables:

```cmake
pico_enable_stdio_usb(pico_blink 1)
pico_enable_stdio_uart(pico_blink 0)
```

It also explicitly enables the baud-rate and vendor USB reset interfaces:

```cmake
PICO_ENABLE_USB_RESET_VIA_BAUD_RATE=1
PICO_ENABLE_USB_RESET_VIA_VENDOR_INTERFACE=1
```

## Independent RAM-layout project

Build the program that reconciles the complete physical SRAM layout, including
the vector/pre-data area, `.data`, `.bss`, heap capacity, scratch banks, and
both core stack reservations:

```sh
./scratch/ram_layout/build.sh
```

Principal outputs:

```text
scratch/ram_layout/build/ram_layout.elf
scratch/ram_layout/build/ram_layout.uf2
```

The linker symbols observed in the compiled image were:

```text
__end__        = 0x20001c84
__HeapLimit    = 0x20040000
__StackBottom  = 0x20041800
__StackTop     = 0x20042000
```

For that exact build, the reported heap capacity is 254844 bytes. Rebuild and
inspect the symbols again after changing the program because addresses can move.

## First flash or recovery with BOOTSEL

The first firmware without a USB reset interface must be installed through the
ROM BOOTSEL loader:

1. Disconnect or reset the Pico.
2. Hold **BOOTSEL** while connecting it, or hold **BOOTSEL** while pressing and
   releasing **RUN/RESET**.
3. Release **BOOTSEL**.
4. Confirm that macOS mounted `/Volumes/RPI-RP2`.

Check the mount:

```sh
mount | grep '/Volumes/RPI-RP2'
diskutil info /Volumes/RPI-RP2
```

Flash a UF2 directly:

```sh
cp scratch/pico_blink/build/pico_blink.uf2 /Volumes/RPI-RP2/
sync
```

The volume normally disappears when the ROM accepts the image and reboots into
the application.

The blink project's script performs the same operation when the volume exists:

```sh
./scratch/pico_blink/flash.sh
```

If copying reports `Permission denied`, inspect the volume rather than assuming
it is read-only:

```sh
mount | grep '/Volumes/RPI-RP2'
ls -ldeO@ /Volumes/RPI-RP2
diskutil info /Volumes/RPI-RP2
```

The successful test volume reported both `Media Read-Only: No` and
`Volume Read-Only: No`. Reconnecting in BOOTSEL mode and retrying resolved the
transient copy failure.

## USB-enabled picotool

The blink build creates a host-native picotool with libusb support at:

```text
scratch/pico_blink/build-picotool-usb/picotool
```

Confirm its version and available USB commands:

```sh
scratch/pico_blink/build-picotool-usb/picotool version
scratch/pico_blink/build-picotool-usb/picotool help load
scratch/pico_blink/build-picotool-usb/picotool help reboot
```

Confirm that it is linked to libusb:

```sh
otool -L scratch/pico_blink/build-picotool-usb/picotool | grep libusb
```

The macOS dependency was discovered through Homebrew and pkg-config:

```sh
brew --prefix libusb
pkg-config --modversion libusb-1.0
pkg-config --cflags --libs libusb-1.0
```

The Pico SDK's internally generated picotool is commonly built without libusb;
use the explicit `build-picotool-usb/picotool` executable for device operations.

## Flash without touching BOOTSEL

Once a firmware with the Pico SDK USB reset interface is running, flash and
execute another image with:

```sh
scratch/pico_blink/build-picotool-usb/picotool load -f -x \
  scratch/ram_layout/build/ram_layout.uf2
```

Options:

```text
-f, --force      Ask compatible running firmware to enter BOOTSEL temporarily.
-x, --execute    Reboot into the newly written application after loading it.
```

For the blink project, the wrapper script selects the mounted BOOTSEL volume
when present and otherwise uses USB-enabled picotool:

```sh
./scratch/pico_blink/flash.sh
```

Discover the board from application mode:

```sh
scratch/pico_blink/build-picotool-usb/picotool info -a
```

When the board is running compatible firmware, picotool reports a USB serial
device and suggests `-f`. It need not appear as an accessible BOOTSEL device in
application mode.

## Restart the installed program

Restart the application from flash without writing a new image:

```sh
scratch/pico_blink/build-picotool-usb/picotool reboot -f -a
```

Options:

```text
-f, --force           Reach the compatible running firmware through USB reset.
-a, --application     Boot the installed application; this is also the default.
```

The USB serial device disappears briefly and returns after reboot.

## USB serial output

List the connected USB CDC serial devices:

```sh
ls -1 /dev/cu.usbmodem*
```

Open the exact device reported by the preceding command:

```sh
screen /dev/cu.usbmodem21301 115200
```

The numeric suffix can change after reconnecting, so do not assume `21301` is
permanent.

`115200` is conventional but does not control USB CDC transfer speed. USB CDC
uses USB packets rather than a physical UART baud clock. Avoid selecting 1200
baud: with reset-via-baud enabled, 1200 is the intentional trigger for rebooting
the Pico into BOOTSEL mode.

The RAM-layout program waits in `stdio_usb_connected()` until the host opens the
CDC port, then prints its output once.

### Exit or recover `screen`

The normal exit sequence is sequential, not simultaneous:

1. Hold **Control** and press **A**.
2. Release both keys.
3. Press `\`.
4. Confirm with `y`.

Exit `screen` before rebooting the board. `screen` does not gracefully handle
the USB serial device disappearing and can leave the terminal in an awkward
state.

If the escape key cannot be sent, open another Terminal tab with **Command-T**:

```sh
screen -ls
screen -S SESSION_ID -X quit
```

For example, the session recovered during validation was listed in the form:

```text
30383.ttys000.jmac
```

Use the current value from `screen -ls`, not that historical identifier.

## Inspect firmware size and symbols

Set short shell variables for the local toolchain and ELF:

```sh
toolchain=vendor/toolchain/expanded/Payload/bin/arm-none-eabi
elf=scratch/pico_blink/build/pico_blink.elf
```

Show overall section sizes:

```sh
"${toolchain}-size" -A "$elf"
"${toolchain}-size" "$elf"
```

Show symbol addresses and sizes:

```sh
"${toolchain}-nm" -n -C "$elf"
"${toolchain}-nm" -S --size-sort -C "$elf"
```

Inspect just the RAM-layout linker symbols:

```sh
vendor/toolchain/expanded/Payload/bin/arm-none-eabi-nm -n \
  scratch/ram_layout/build/ram_layout.elf \
  | grep -E '(__end__|__HeapLimit|__StackBottom|__StackTop)$'
```

Check an output's file type:

```sh
file scratch/ram_layout/build/ram_layout.elf
file scratch/ram_layout/build/ram_layout.uf2
```

The ELF contains debugging sections that are not copied into flash. Use the BIN
or loadable ELF sections—not the ELF file's filesystem size—to determine flash
consumption. UF2 is also larger than the raw firmware because it is a block
transfer container.

## Disassemble the firmware

Disassemble all executable code with interleaved source where available:

```sh
vendor/toolchain/expanded/Payload/bin/arm-none-eabi-objdump -d -S \
  scratch/pico_blink/build/pico_blink.elf
```

Disassemble one function:

```sh
vendor/toolchain/expanded/Payload/bin/arm-none-eabi-objdump -d -S \
  --disassemble=stdio_init_all \
  scratch/pico_blink/build/pico_blink.elf
```

Inspect the caller:

```sh
vendor/toolchain/expanded/Payload/bin/arm-none-eabi-objdump -d \
  --disassemble=main \
  scratch/pico_blink/build/pico_blink.elf
```

In the measured release build, `stdio_init_all` was:

```asm
push {r4, lr}
bl   stdio_usb_init
pop  {r4, pc}
```

Measured program-space costs were:

```text
BL instruction in main          4 bytes
stdio_init_all body             8 bytes
Call site plus wrapper         12 bytes
stdio_usb_init body           228 bytes
```

For a zero-wait-state Cortex-M0+, the direct call/return overhead was:

```text
outer BL                         3 cycles
PUSH {r4, lr}                    3 cycles
inner BL                         3 cycles
POP {r4, pc}                     5 cycles
total overhead                  14 cycles
```

At the RP2040's default 125 MHz system clock, 14 cycles is nominally 112 ns.
This excludes `stdio_usb_init` itself. Its execution time is variable because it
calls TinyUSB, configures peripherals and interrupts, takes conditional paths,
and executes through the RP2040 XIP flash cache.

## What `stdio_init_all()` enables here

Because this firmware enables USB stdio and disables UART stdio,
`stdio_init_all()` reduces conceptually to:

```c
bool stdio_init_all(void) {
    return stdio_usb_init();
}
```

`stdio_usb_init()`:

1. Checks that it is running on the default alarm pool's core.
2. Initializes the TinyUSB root port.
3. Initializes the USB stdio mutex if needed.
4. Claims and enables a low-priority software interrupt.
5. Arranges periodic or IRQ-driven calls to `tud_task()`.
6. Registers the USB stdio driver.
7. Returns success without waiting for a host connection in this configuration.

The linked Pico USB reset component adds the vendor interface used by
`picotool -f`. `stdio_init_all()` initializes the USB machinery; it does not
itself flash or reset the board.

## Useful diagnostics

Check whether the BOOTSEL volume exists:

```sh
test -d /Volumes/RPI-RP2 && echo BOOTSEL || echo application-mode
```

Inspect connected USB devices on macOS:

```sh
system_profiler SPUSBDataType
```

Check the project scripts for shell syntax errors:

```sh
bash -n scratch/pico_blink/build.sh scratch/pico_blink/flash.sh
bash -n scratch/ram_layout/build.sh
```

Confirm that local scratch content is ignored:

```sh
git check-ignore -v scratch/ram_layout/main.c
git check-ignore -v scratch/pico_blink/build/pico_blink.uf2
```
