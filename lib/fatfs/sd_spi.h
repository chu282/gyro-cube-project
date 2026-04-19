#pragma once
#include <stdint.h>
#include <stdbool.h>

bool sd_init(void);
bool sd_read_block(uint32_t block, uint8_t *buffer);