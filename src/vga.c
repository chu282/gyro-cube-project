#include "pico/stdlib.h"
#include <stdio.h>
#include <stdlib.h>
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hsync.pio.h"
#include "vsync.pio.h"
#include "rgb.pio.h"
#include "vga.h"

/*
Most of the VGA implementation is taken straight from https://vanhunteradams.com/Pico/VGA/VGA.html, 
including hsync.pio, vsync.pio, and rgb.pio. 
*/

// screen
#define SCREEN_WIDTH 1024
#define SCREEN_HEIGHT 600
// #define SCREEN_WIDTH 640
// #define SCREEN_HEIGHT 360

// gpio pins
#define GPIO_HSYNC    10
#define GPIO_VSYNC    11
#define GPIO_RED      12
#define GPIO_BLUE     13
#define GPIO_GREEN_LO 14
#define GPIO_GREEN_HI 15

// VGA timing constants
#define FRONTPORCH 48
#define H_ACTIVE (SCREEN_WIDTH + FRONTPORCH - 1) // (active + frontporch - 1) - one cycle delay for mov
#define V_ACTIVE (SCREEN_HEIGHT - 1) // (active - 1)
#define RGB_ACTIVE (SCREEN_WIDTH / 2 - 1) // (horizontal active)/2 - 1
#define TXCOUNT (SCREEN_WIDTH * SCREEN_HEIGHT / 2) // total pixels / 2

uint8_t vga_data[TXCOUNT];
char* address_pointer = &vga_data[0];

// A function for drawing a pixel with a specified color.
// Note that because information is passed to the PIO state machines through
// a DMA channel, we only need to modify the contents of the array and the
// pixels will be automatically updated on the screen.
void draw_pixel(int x, int y, char color) {
    color = color & 0x0F;
    // Range checks
    if (x > SCREEN_WIDTH - 1) x = SCREEN_WIDTH - 1;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (y > SCREEN_HEIGHT - 1) y = SCREEN_HEIGHT - 1;

    // Which pixel is it?
    int pixel = ((SCREEN_WIDTH * y) + x);

    if (pixel & 1) {
        vga_data[pixel>>1] = (vga_data[pixel>>1] & 0x0F) | (color << 4);
    }
    else {
        vga_data[pixel>>1] = (vga_data[pixel>>1] & 0xF0) | color;
    }
}

void draw_rect(int x, int y, int w, int h, char color) {
  for (int i = 0; i < w; i++) {
    for (int j = 0; j < h; j++) {
        draw_pixel(x + i, y + j, color);
    }
  }
}

void draw_colorboard() {
    for (int i = 0; i < 16; i++) {
        for (int j = 0; j < 16; j++) {
            draw_rect(64 * i, 38 * j, 64, 38, (i + j) % 16);
        }
    }
}

// DMA channels - 0 sends color data, 1 reconfigures and restarts 0
void init_vga() {
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
    pio_sm_claim(pio, hsync_sm);
    pio_sm_claim(pio, vsync_sm);
    pio_sm_claim(pio, rgb_sm);

    // Call the initialization functions that are defined within each PIO file.
    // Why not create these programs here? By putting the initialization function in
    // the pio file, then all information about how to use/setup that state machine
    // is consolidated in one place. Here in the C, we then just import and use it.
    hsync_program_init(pio, hsync_sm, hsync_offset, GPIO_HSYNC);
    vsync_program_init(pio, vsync_sm, vsync_offset, GPIO_VSYNC);
    rgb_program_init(pio, rgb_sm, rgb_offset, GPIO_RED);

    int rgb_chan_0 = dma_claim_unused_channel(true);
    int rgb_chan_1 = dma_claim_unused_channel(true);

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

    // Initialize PIO state machine counters. This passes the information to the state machines
    // that they retrieve in the first 'pull' instructions, before the .wrap_target directive
    // in the assembly. Each uses these values to initialize some counting registers.
    pio_sm_put_blocking(pio, hsync_sm, H_ACTIVE);
    pio_sm_put_blocking(pio, vsync_sm, V_ACTIVE);
    pio_sm_put_blocking(pio, rgb_sm, RGB_ACTIVE);


    // Start the two pio machine IN SYNC
    // Note that the RGB state machine is running at full speed,
    // so synchronization doesn't matter for that one. But, we'll
    // start them all simultaneously anyway.
    pio_enable_sm_mask_in_sync(pio, ((1u << hsync_sm) | (1u << vsync_sm) | (1u << rgb_sm)));

    // Start DMA channel 0. Once started, the contents of the pixel color array
    // will be continously DMA's to the PIO machines that are driving the screen.
    // To change the contents of the screen, we need only change the contents
    // of that array.
    dma_start_channel_mask((1u << rgb_chan_0)) ;
}