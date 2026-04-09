#include "pico/stdlib.h"
#include <stdio.h>
#include <stdlib.h>
#include "hardware/adc.h"

void init_adc () {
    gpio_init(45);

    gpio_set_dir(45, GPIO_IN);

    adc_hw->cs();
}