#include "pico/stdlib.h"
#include <stdio.h>
#include <stdlib.h>
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "vga.h"
#include "imu.h"
#include "cube.h"
#include "cube_math.h"
#include "zoom.h"

int main() {
    stdio_init_all();

    // initialize VGA
    init_vga();
    cube_irq_init();
    cube_init();
    cube_run();
    // draw_colorboard();

    // draw_line(100, 100, 200, 200, WHITE);
    
    for (;;) {
        printf("Hello world!\n");
        sleep_ms(1000);
    }
}