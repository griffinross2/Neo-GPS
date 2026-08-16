#pragma once

#include "Vgps_tb.h"

// Initialise the SPI host and spawn the test program thread. The body starts
// running at the first sim_service() call and blocks its way through the test.
void test_program_init(VerilatedContext *const contextp, Vgps_tb *const top);
