# Board platforms

The numerical implementation is portable C in `src/`; `src/arch/` currently
identifies the selected backend, with no optimized kernels. Platform directories
contain board integration and must use the public library API.

| Platform | Current evidence |
| --- | --- |
| [Raspberry Pi Pico](raspberrypi/pico/README.md) | RP2040 firmware flashed; focused test suite passed over USB serial |
| [TI F28P55x](ti/f28p55x/README.md) | Library and flash benchmark compile/link with TI 25.11.1.LTS; execution pending |
| [ST STM32G474](st/stm32g474/README.md) | Toolchain/preset configured; no compiler or hardware verification |
| [Microchip dsPIC33A](microchip/dspic33a/README.md) | Toolchain/preset configured; no compiler or hardware verification |

No physical-board result is claimed for the TI, ST, or Microchip targets. STM32
and dsPIC firmware ports are deferred beyond the first snapshot. Board SDK code
stays outside the core library; the Pico build defaults to dependencies placed
locally in the ignored `vendor/` directory.
