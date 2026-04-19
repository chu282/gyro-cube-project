#include "ff.h"
#include "diskio.h"
#include "sd_spi.h"
#include <string.h>

/* Drive 0 = SD card */

#ifndef BYTE
typedef unsigned char BYTE;
#endif

#ifndef UINT
typedef unsigned int UINT;
#endif

#ifndef DWORD
typedef uint32_t DWORD;
#endif

DSTATUS disk_initialize(BYTE pdrv) {
    if (pdrv != 0) return STA_NOINIT;
    return sd_init() ? 0 : STA_NOINIT;
}

DSTATUS disk_status(BYTE pdrv) {
    if (pdrv != 0) return STA_NOINIT;
    return 0;
}

DRESULT disk_read(
    BYTE pdrv,
    BYTE *buff,
    LBA_t sector,
    UINT count
) {
    if (pdrv != 0) return RES_PARERR;

    for (UINT i = 0; i < count; i++) {
        if (!sd_read_block(sector + i, buff + (i * 512)))
            return RES_ERROR;
    }

    return RES_OK;
}

DRESULT disk_write(
    BYTE pdrv,
    const BYTE *buff,
    LBA_t sector,
    UINT count
) {
    return RES_WRPRT;
}

DRESULT disk_ioctl(
    BYTE pdrv,
    BYTE cmd,
    void *buff
) {
    if (pdrv != 0) return RES_PARERR;

    switch (cmd) {
        case CTRL_SYNC:
            return RES_OK;

        case GET_SECTOR_SIZE:
            *(WORD*)buff = 512;
            return RES_OK;

        case GET_SECTOR_COUNT:
            *(DWORD*)buff = 32768; // fake safe value (adjust if needed)
            return RES_OK;

        default:
            return RES_PARERR;
    }
}