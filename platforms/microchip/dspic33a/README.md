# dsPIC33AK512MPS506 Curiosity Nano platform

**Status: library cross-compiled; API consumer link checked. Physical execution
and benchmarking remain pending.**

Validated with Microchip XC-DSC 4.00.00 with dsPIC33AK-MP DFP 1.6.273.
See [build instructions](../../../docs/BUILDING.md) and
[compiler evidence](../../../docs/VALIDATION.md#stm32g474-and-dspic33a-compiler-evidence).
The consumer link check is not a board firmware port.

Planned integration point for dsPIC33A benchmark firmware. Keep configuration
bits, startup/linker settings, on-board debugger integration, timers, GPIO,
and UART code here. ISA-level kernels belong in `src/arch/dspic33a/`.
