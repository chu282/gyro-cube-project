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

    // ====== Test Display ======
    // draw_rect(100, 100, 100, 100, WHITE);
    // draw_colorboard();

    // i2c_init(i2c0, 400000);
    // gpio_set_function(4, GPIO_FUNC_I2C);
    // gpio_set_function(5, GPIO_FUNC_I2C);
    // gpio_pull_up(4);
    // gpio_pull_up(5);

    // for (uint8_t addr = 8; addr < 120; addr++) {
    //     uint8_t buf;
    //     int ret = i2c_read_blocking(i2c0, addr, &buf, 1, false);
    //     if (ret >= 0) {
    //         printf("Found device at 0x%02X\n", addr);
    //     }
    // }
    
    printf("heljkfldsjsdf");
    cube_init();
    cube_run();
    // draw_colorboard();

    // draw_line(100, 100, 200, 200, WHITE);
    
    for (;;) {
        printf("Hello world!\n");
        sleep_ms(1000);
    }
}