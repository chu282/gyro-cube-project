#include "sd_spi.h"
#include "hardware/spi.h"
#include "pico/stdlib.h"
#include <string.h>

/* ---------- CONFIG ---------- */
#define SD_SPI spi0
#define PIN_MISO 32
#define PIN_MOSI 35
#define PIN_SCK  34
#define PIN_CS   33

/* ---------- LOW LEVEL ---------- */

static inline void cs_low()  { gpio_put(PIN_CS, 0); }
static inline void cs_high() { gpio_put(PIN_CS, 1); }

static uint8_t spi_txrx(uint8_t data) {
    uint8_t rx;
    spi_write_read_blocking(SD_SPI, &data, &rx, 1);
    return rx;
}

static void spi_dummy_clocks(int n) {
    for (int i = 0; i < n; i++) spi_txrx(0xFF);
}

/* ---------- SD COMMANDS ---------- */

static uint8_t sd_cmd(uint8_t cmd, uint32_t arg, uint8_t crc) {
    spi_txrx(0xFF);
    spi_txrx(0x40 | cmd);
    spi_txrx(arg >> 24);
    spi_txrx(arg >> 16);
    spi_txrx(arg >> 8);
    spi_txrx(arg);
    spi_txrx(crc);

    for (int i = 0; i < 10; i++) {
        uint8_t r = spi_txrx(0xFF);
        if (!(r & 0x80)) return r;
    }
    return 0xFF;
}

/* ---------- INIT ---------- */

bool sd_init(void) {
    spi_init(SD_SPI, 400 * 1000);

    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);

    gpio_init(PIN_CS);
    gpio_set_dir(PIN_CS, GPIO_OUT);
    cs_high();

    sleep_ms(20);

    cs_low();
    spi_dummy_clocks(10);
    cs_high();

    /* CMD0 */
    cs_low();
    uint8_t r = sd_cmd(0, 0, 0x95);
    cs_high();
    if (r != 0x01) return false;

    /* CMD8 */
    cs_low();
    r = sd_cmd(8, 0x1AA, 0x87);
    cs_high();

    /* ACMD41 loop (simplified) */
    for (int i = 0; i < 1000; i++) {
        cs_low();
        sd_cmd(55, 0, 0);
        cs_high();

        cs_low();
        r = sd_cmd(41, 0x40000000, 0);
        cs_high();

        if (r == 0x00) break;
        sleep_ms(10);
    }

    spi_set_baudrate(SD_SPI, 12 * 1000 * 1000);
    return true;
}

/* ---------- READ BLOCK (512B) ---------- */

bool sd_read_block(uint32_t block, uint8_t *buffer) {
    cs_low();

    if (sd_cmd(17, block, 0) != 0x00) {
        cs_high();
        return false;
    }

    while (spi_txrx(0xFF) != 0xFE);

    for (int i = 0; i < 512; i++) {
        buffer[i] = spi_txrx(0xFF);
    }

    spi_txrx(0xFF);
    spi_txrx(0xFF);

    cs_high();
    spi_txrx(0xFF);

    return true;
}