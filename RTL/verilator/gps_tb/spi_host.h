#pragma once

#include "Vgps_tb.h"
#include <stdint.h>

// --- Sim thread side --------------------------------------------------------

// Drive the SPI pins to idle, before the simulation loop starts.
void spi_init(Vgps_tb *const top);

// Advance the SPI bit engine. Call once per tick, before top->eval().
void spi_task(Vgps_tb *const top);

// --- Test thread side -------------------------------------------------------

// Blocking transactions: these return once the transaction has completed on the
// wire. Only call these from the test program thread.
void spi_write(Vgps_tb *const top, uint16_t addr, uint16_t data);
uint16_t spi_read(Vgps_tb *const top, uint16_t addr);
