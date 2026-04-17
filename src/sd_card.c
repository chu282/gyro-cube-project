#include "sd_card.h"

// ---------------------------------------------------------------------------
// SD card driver
//
// This file relies on the no-OS-FatFS-SD-SPI-RPi-Pico library by carlk3:
//   https://github.com/carlk3/no-OS-FatFS-SD-SPI-RPi-Pico
//
// That library provides:
//   - sd_init_driver()   (hardware/SPI initialisation)
//   - ff.h + FatFS       (f_open, f_read, f_close, FIL, FR_OK …)
//
// Add the library to your CMakeLists.txt:
//   add_subdirectory(no-OS-FatFS-SD-SPI-RPi-Pico/FatFs_SPI build)
//   target_link_libraries(<your_target> FatFs_SPI)
//
// The SPI pins and SD card slot are configured in hw_config.c (part of
// the library).  A minimal hw_config.c for SPI0 on the default Pico pins:
//
//   SPI0: SCK  → GP2
//         MOSI → GP3
//         MISO → GP4
//         CS   → GP5
//
// If you are using different pins, edit hw_config.c in the library folder.
// ---------------------------------------------------------------------------

// sd_init_driver() is implemented by the carlk3 library.
// No additional code is needed here — the linker resolves it from that library.
