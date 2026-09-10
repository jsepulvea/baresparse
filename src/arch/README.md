# Architecture backends

This directory contains ISA-level implementations selected by `BRSP_ARCH`.
They must not depend on a board SDK, startup code, an operating system, or a
linker script.

The portable backend is the correctness baseline. C28x, Cortex-M4, and
dsPIC33A backends can progressively replace individual kernels after measured
performance work. Board-specific clocks, timers, GPIO, UART, and flashing
belong under `platforms/` or in a separate firmware application.
