#include "brsp_benchmark.h"
#include "brsp_benchmark_build.h"
#include "device.h"
#include "driverlib.h"

/* The SDK linker maps ramgs0 to RAMGS0. Keep the large context out of the
 * small default .bss region. This is benchmark state, not library state. */
#pragma DATA_SECTION(brsp_board_context, "ramgs0")
static brsp_bench_context brsp_board_context;
volatile unsigned int brsp_board_result = 0U;

_Static_assert(CHAR_BIT == 16, "This firmware expects C28x 16-bit C bytes");
_Static_assert(sizeof(brsp_real) == 2, "C28x float size");
_Static_assert(sizeof(brsp_index) == 2, "C28x index size");
_Static_assert(DEVICE_SYSCLK_FREQ == 150000000UL, "Revalidate timing after a clock change");

uint32_t brsp_bench_now(void) {
    /* CPUTimer counts down. Unsigned negation gives a modulo-2^32 up counter. */
    return (uint32_t)(0UL - CPUTimer_getTimerCount(CPUTIMER0_BASE));
}

void brsp_bench_write(const char *text) {
    while (*text != '\0') {
        if (*text == '\n')
            SCI_writeCharBlockingNonFIFO(SCIA_BASE, '\r');
        SCI_writeCharBlockingNonFIFO(SCIA_BASE, (uint16_t)*text++);
    }
}

int main(void) {
    Device_init();
    Device_initGPIO();
    DINT;
    Interrupt_initModule();
    Interrupt_initVectorTable();
    GPIO_setPinConfig(DEVICE_GPIO_CFG_LED1);
    GPIO_setPinConfig(DEVICE_GPIO_CFG_LED2);
    GPIO_setDirectionMode(DEVICE_GPIO_PIN_LED1, GPIO_DIR_MODE_OUT);
    GPIO_setDirectionMode(DEVICE_GPIO_PIN_LED2, GPIO_DIR_MODE_OUT);
    GPIO_writePin(DEVICE_GPIO_PIN_LED1, 1U);
    GPIO_writePin(DEVICE_GPIO_PIN_LED2, 1U);
    GPIO_setPinConfig(DEVICE_GPIO_CFG_SCIRXDA);
    GPIO_setPinConfig(DEVICE_GPIO_CFG_SCITXDA);
    GPIO_setQualificationMode(DEVICE_GPIO_PIN_SCIRXDA, GPIO_QUAL_ASYNC);
    SCI_performSoftwareReset(SCIA_BASE);
    SCI_setConfig(SCIA_BASE, DEVICE_LSPCLK_FREQ, 115200U,
                  SCI_CONFIG_WLEN_8 | SCI_CONFIG_STOP_ONE | SCI_CONFIG_PAR_NONE);
    SCI_disableLoopback(SCIA_BASE);
    SCI_disableFIFO(SCIA_BASE);
    SCI_enableModule(SCIA_BASE);
    SCI_resetChannels(SCIA_BASE);
    CPUTimer_stopTimer(CPUTIMER0_BASE);
    CPUTimer_setPeriod(CPUTIMER0_BASE, UINT32_MAX);
    CPUTimer_setPreScaler(CPUTIMER0_BASE, 0U);
    CPUTimer_disableInterrupt(CPUTIMER0_BASE);
    CPUTimer_setEmulationMode(CPUTIMER0_BASE, CPUTIMER_EMULATIONMODE_RUNFREE);
    CPUTimer_reloadTimerCounter(CPUTIMER0_BASE);
    CPUTimer_startTimer(CPUTIMER0_BASE);
    /* Allow a serial reader opened before reset to settle; never timed. */
    DEVICE_DELAY_US(1000000U);
    brsp_bench_write("# platform=LAUNCHXL-F28P55X\n# timer=CPUTimer0 SYSCLK prescaler 0\n");
    brsp_bench_write("# revision=" BRSP_BENCH_REVISION "\n# compiler=" BRSP_BENCH_COMPILER "\n");
    brsp_bench_write("# flags=" BRSP_BENCH_FLAGS "\n# sdk_revision=" BRSP_BENCH_SDK_REVISION "\n");
    brsp_bench_write("# interrupts=masked throughout experiment\n# code_placement=flash; flash "
                     "init in RAMLS0\n");
    brsp_bench_write("# data_placement=RAMGS0 context; RAMM1 stack\n# stack_usage=not measured\n");
    brsp_bench_metadata("timer_hz", DEVICE_SYSCLK_FREQ);
    brsp_bench_metadata("sysclk_hz", DEVICE_SYSCLK_FREQ);
    brsp_bench_metadata("lspclk_hz", DEVICE_LSPCLK_FREQ);
    brsp_bench_metadata("flash_waitstates", DEVICE_FLASH_WAITSTATES);
    brsp_bench_metadata("stack_allocated_c_bytes", 0x400U);
    brsp_board_result = brsp_bench_run(&brsp_board_context) == 0 ? 1U : 2U;
    GPIO_writePin(brsp_board_result == 1U ? DEVICE_GPIO_PIN_LED2 : DEVICE_GPIO_PIN_LED1, 0U);
    for (;;) { /* Result and UART data remain observable without a debugger. */
    }
}
