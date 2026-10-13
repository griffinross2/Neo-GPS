#pragma once

#include <stdint.h>

void parallel_interface_init();
void parallel_interface_write(uint16_t addr, uint32_t data);
uint32_t parallel_interface_read(uint16_t addr);