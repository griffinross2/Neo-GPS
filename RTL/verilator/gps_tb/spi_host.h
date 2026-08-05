#pragma once

#include "Vgps_tb.h"
#include <stdint.h>

void spi_init(Vgps_tb *const top);
void spi_write(Vgps_tb *const top, uint8_t addr, uint8_t data);
void spi_read(Vgps_tb *const top, uint8_t addr);
void spi_task(Vgps_tb *const top);
bool spi_is_idle();
uint8_t spi_get_read_data();