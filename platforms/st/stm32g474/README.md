# NUCLEO-G474RE platform

**Status: library cross-compiled; API consumer link checked. Physical execution
and benchmarking remain pending.**

Validated with Arm GNU Toolchain 15.2.Rel1 (GCC 15.2.1).
See [build instructions](../../../docs/BUILDING.md) and
[compiler evidence](../../../docs/VALIDATION.md#stm32g474-and-dspic33a-compiler-evidence).
The consumer link check is not a board firmware port.

Planned integration point for STM32G474 benchmark firmware. Keep CMSIS device
startup, the linker script, ST-LINK flash commands, clocks, DWT/timer access,
GPIO, and UART code here. Cortex-M4 kernels belong in `src/arch/cortex_m4/`.
