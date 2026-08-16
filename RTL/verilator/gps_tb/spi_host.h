#pragma once

#include "Vgps_tb.h"
#include <stdint.h>

void spi_init(Vgps_tb *const top);
void spi_write(Vgps_tb *const top, uint16_t addr, uint16_t data);
void spi_read(Vgps_tb *const top, uint16_t addr);
void spi_task(Vgps_tb *const top);
bool spi_is_idle();
uint16_t spi_get_read_data();