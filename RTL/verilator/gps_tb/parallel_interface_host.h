#pragma once

#include "Vgps_tb.h"
#include <stdint.h>

// Host model for the 16-bit synchronous parallel interface (parallel_interface.sv),
// mirroring the RP2350 PIO program in RP2350/pio/parallel_interface.pio.
//
// One transaction is four if_clk cycles with CS low:
//   cycle 0: {rw, addr[14:0]}   (rw = 1 for read)
//   cycle 1: dummy / bus turnaround
//   cycle 2: data[31:16]
//   cycle 3: data[15:0]
// Both sides change the bus on the falling edge and sample on the rising edge.

// --- Sim thread side --------------------------------------------------------

// Drive the interface pins to idle, before the simulation loop starts.
void pi_init(Vgps_tb *const top);

// Advance the bus engine. Call once per tick, before top->eval().
void pi_task(Vgps_tb *const top);

// --- Test thread side -------------------------------------------------------

// Blocking transactions: these return once the transaction has completed on the
// wire. Only call these from the test program thread.
void pi_write(Vgps_tb *const top, uint16_t addr, uint32_t data);
uint32_t pi_read(Vgps_tb *const top, uint16_t addr);
