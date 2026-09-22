#include "basic_suite.h"

#include "pico/stdio.h"
#include "pico/stdio_usb.h"
#include "pico/stdlib.h"

#ifndef PICO_DEFAULT_LED_PIN
#error "The selected Pico board does not define an onboard LED pin"
#endif

static void brsp_wait_for_usb(void) {
    bool led_on = false;

    while (!stdio_usb_connected()) {
        led_on = !led_on;
        gpio_put(PICO_DEFAULT_LED_PIN, led_on);
        sleep_ms(500U);
    }

    gpio_put(PICO_DEFAULT_LED_PIN, false);
    sleep_ms(100U);
}

static void brsp_indicate_failure(void) {
    for (;;) {
        gpio_put(PICO_DEFAULT_LED_PIN, true);
        sleep_ms(125U);
        gpio_put(PICO_DEFAULT_LED_PIN, false);
        sleep_ms(125U);
    }
}

int main(void) {
    int result;

    stdio_init_all();
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    gpio_put(PICO_DEFAULT_LED_PIN, false);

    brsp_wait_for_usb();
    result = brsp_basic_tests_run();
    stdio_flush();

    if (result != 0) {
        brsp_indicate_failure();
    }

    gpio_put(PICO_DEFAULT_LED_PIN, true);
    for (;;) {
        tight_loop_contents();
    }
}
