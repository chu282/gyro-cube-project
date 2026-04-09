#include "pico/stdlib.h"
#include <stdio.h>
#include <stdlib.h>
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "vga.c"
#include "adc.c"

int main() {
    stdio_init_all();

    // initialize VGA
    init_VGA();
    init_adc(); // adc_fifo_out will contain voltage levels from 0-4095

    for (;;) {
        printf("Hello world!\n");
        sleep_ms(1000);
    }
}