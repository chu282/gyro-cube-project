#pragma once

#include <stdbool.h>

// FatFS — provides FIL, UINT, FRESULT, FR_OK, FA_READ,
// f_open(), f_read(), f_close(), etc.
// Requires the no-OS-FatFS-SD-SPI-RPi-Pico library to be present in the build.
#include "ff.h" 

// Initialize the SD card SPI driver and mount the FatFS volume.
// Returns true on success, false if the card is absent or unreadable.
// Must be called before any f_open() / f_read() / f_close() calls.
bool sd_init_driver(void);
