#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/dma.h"
#include "hardware/gpio.h"
// #include "chardisp.h"

// Make sure to set these in main.c
extern const int SPI_DISP_SCK; extern const int SPI_DISP_CSn; extern const int SPI_DISP_TX;

/***************************************************************** */

// "chardisp" stands for character display, which can be an LCD or OLED
void init_chardisp_pins() {
    // fill in
    gpio_set_function(SPI_DISP_SCK, GPIO_FUNC_SPI);
    gpio_set_function(SPI_DISP_CSn, GPIO_FUNC_SPI);
    gpio_set_function(SPI_DISP_TX, GPIO_FUNC_SPI);
    spi_init(spi1, 10000);
    spi_set_format(spi1, 9, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
}

void send_spi_cmd(spi_inst_t* spi, uint16_t value) {
    // fill in
    while (spi1_hw->sr & SPI_SSPSR_BSY_BITS);
    spi_write16_blocking(spi, &value, 1);
}

void send_spi_data(spi_inst_t* spi, uint16_t value) {
    // fill in
    send_spi_cmd(spi, value | 0x100);
}

void cd_init() {
    // fill in
    // sleep
    sleep_ms(50);
    
    // function set
    send_spi_cmd(spi1, 0x3C);
    sleep_us(40);
    
    // display on/off
    send_spi_cmd(spi1, 0x0C);
    sleep_us(40);
    
    // clear display
    send_spi_cmd(spi1, 0x01);
    sleep_ms(2);
    
    // entry mode set
    send_spi_cmd(spi1, 0x06);
    sleep_us(40);
}

void cd_display1(const char *str) {
    // fill in
    // move to first line
    send_spi_cmd(spi1, 0x80);

    // write data
    for (int i = 0; i < 16; i++) {
        send_spi_data(spi1, (uint16_t)str[i]);
        sleep_us(45);
    }
}
void cd_display2(const char *str) {
    // fill in
    // move to second line
    send_spi_cmd(spi1, 0x80 | 0x40);
    
    // write data
    for (int i = 0; i < 16; i++) {
        send_spi_data(spi1, (uint16_t)str[i]);
        sleep_us(45);
    }
}

/***************************************************************** */