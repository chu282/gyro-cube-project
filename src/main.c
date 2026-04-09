#include "pico/stdlib.h"
#include <stdio.h>
#include <stdlib.h>
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "vga.c"

int main() {
    stdio_init_all();

    // initialize VGA
    init_vga();

    for (;;) {
        printf("Hello world!\n");
        sleep_ms(1000);
    }
}