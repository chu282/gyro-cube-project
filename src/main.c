#include "pico/stdlib.h"
#include <stdio.h>
#include <stdlib.h>
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "vga.h"

int main() {
    stdio_init_all();

    // initialize VGA
    init_vga();

    draw_rect(100, 100, 100, 100, WHITE);
    draw_colorboard();

    for (;;) {
        printf("Hello world!\n");
        sleep_ms(1000);
    }
}