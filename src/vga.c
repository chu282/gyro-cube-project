#include "pico/stdlib.h"
#include <stdio.h>
#include <stdlib.h>
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hsync.pio.h"
#include "vsync.pio.h"
#include "rgb.pio.h"

// screen
#define SCREEN_WIDTH 1024
#define SCREEN_HEIGHT 600

// gpio pins
#define GPIO_HSYNC    10
#define GPIO_VSYNC    11
#define GPIO_RED      12
#define GPIO_BLUE     13
#define GPIO_GREEN_LO 14
#define GPIO_GREEN_HI 15

// VGA timing constants
#define FRONTPORCH 16
#define H_ACTIVE (SCREEN_WIDTH + FRONTPORCH - 1) // (active + frontporch - 1) - one cycle delay for mov
#define V_ACTIVE (SCREEN_HEIGHT - 1) // (active - 1)
#define RGB_ACTIVE (SCREEN_WIDTH / 2 - 1) // (horizontal active)/2 - 1
#define TXCOUNT (SCREEN_WIDTH * SCREEN_HEIGHT / 2) // total pixels / 2

// colors
#define BLACK        0   // 0000
#define RED          1   // 0001
#define BLUE         2   // 0010
#define MAGENTA      3   // 0011 (R+B)
#define DARK_GREEN   4   // 0100 (G_Lo)
#define DARK_YELLOW  5   // 0101 (R+G_Lo)
#define DARK_CYAN    6   // 0110 (B+G_Lo)
#define DARK_GRAY    7   // 0111 (R+B+G_Lo)
#define MED_GREEN    8   // 1000 (G_Hi)
#define ORANGE       9   // 1001 (R+G_Hi)
#define LIGHT_BLUE   10  // 1010 (B+G_Hi)
#define PINK         11  // 1011 (R+B+G_Hi)
#define BRIGHT_GREEN 12  // 1100 (G_Lo+G_Hi)
#define YELLOW       13  // 1101 (R+G_Lo+G_Hi)
#define CYAN         14  // 1110 (B+G_Lo+G_Hi)
#define WHITE        15  // 1111 (All Pins)

char vga_data[TXCOUNT];
char* address_pointer = &vga_data[0];

// A function for drawing a pixel with a specified color.
// Note that because information is passed to the PIO state machines through
// a DMA channel, we only need to modify the contents of the array and the
// pixels will be automatically updated on the screen.
void drawPixel(int x, int y, char color) {
    // Range checks
    if (x > SCREEN_WIDTH - 1) x = SCREEN_WIDTH - 1;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (y > SCREEN_HEIGHT - 1) y = SCREEN_HEIGHT - 1;

    // Which pixel is it?
    int pixel = ((SCREEN_WIDTH * y) + x);

    if (pixel & 1) {
        vga_data[pixel>>1] |= (color << 4);
    }
    else {
        vga_data[pixel>>1] |= (color);
    }
}

// DMA channels - 0 sends color data, 1 reconfigures and restarts 0
void init_VGA() {
    PIO pio = pio0;

    // Our assembled program needs to be loaded into this PIO's instruction
    // memory. This SDK function will find a location (offset) in the
    // instruction memory where there is enough space for our program. We need
    // to remember these locations!
    //
    // We only have 32 instructions to spend! If the PIO programs contain more than
    // 32 instructions, then an error message will get thrown at these lines of code.
    //
    // The program name comes from the .program part of the pio file
    // and is of the form <program name_program>
    uint hsync_offset = pio_add_program(pio, &hsync_program);
    uint vsync_offset = pio_add_program(pio, &vsync_program);
    uint rgb_offset = pio_add_program(pio, &rgb_program);

    // Manually select a few state machines from pio instance pio0.
    uint hsync_sm = 0;
    uint vsync_sm = 1;
    uint rgb_sm = 2;

    // Call the initialization functions that are defined within each PIO file.
    // Why not create these programs here? By putting the initialization function in
    // the pio file, then all information about how to use/setup that state machine
    // is consolidated in one place. Here in the C, we then just import and use it.
    hsync_program_init(pio, hsync_sm, hsync_offset, GPIO_HSYNC);
    vsync_program_init(pio, vsync_sm, vsync_offset, GPIO_VSYNC);
    rgb_program_init(pio, rgb_sm, rgb_offset, GPIO_RED);

    int rgb_chan_0 = 0;
    int rgb_chan_1 = 1;

    // Channel Zero (sends color data to PIO VGA machine)
    dma_channel_config c0 = dma_channel_get_default_config(rgb_chan_0);  // default configs
    channel_config_set_transfer_data_size(&c0, DMA_SIZE_8);              // 8-bit txfers
    channel_config_set_read_increment(&c0, true);                        // yes read incrementing
    channel_config_set_write_increment(&c0, false);                      // no write incrementing
    channel_config_set_dreq(&c0, DREQ_PIO0_TX2);                         // DREQ_PIO0_TX2 pacing (FIFO)
    channel_config_set_chain_to(&c0, rgb_chan_1);                        // chain to other channel

    dma_channel_configure(
        rgb_chan_0,                 // Channel to be configured
        &c0,                        // The configuration we just created
        &pio->txf[rgb_sm],          // write address (RGB PIO TX FIFO)
        &vga_data,                  // The initial read address (pixel color array)
        TXCOUNT,                    // Number of transfers; in this case each is 1 byte.
        false                       // Don't start immediately.
    );

    // Channel One (reconfigures the first channel)
    dma_channel_config c1 = dma_channel_get_default_config(rgb_chan_1);   // default configs
    channel_config_set_transfer_data_size(&c1, DMA_SIZE_32);              // 32-bit txfers
    channel_config_set_read_increment(&c1, false);                        // no read incrementing
    channel_config_set_write_increment(&c1, false);                       // no write incrementing
    channel_config_set_chain_to(&c1, rgb_chan_0);                         // chain to other channel

    dma_channel_configure(
        rgb_chan_1,                         // Channel to be configured
        &c1,                                // The configuration we just created
        &dma_hw->ch[rgb_chan_0].read_addr,  // Write address (channel 0 read address)
        &address_pointer,                   // Read address (POINTER TO AN ADDRESS)
        1,                                  // Number of transfers, in this case each is 4 byte
        false                               // Don't start immediately.
    );
}