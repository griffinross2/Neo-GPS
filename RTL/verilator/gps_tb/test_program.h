#pragma once

#include "Vgps_tb.h"

void test_program_init(VerilatedContext *const contextp, Vgps_tb *const top);
void test_program_start_cycle(VerilatedContext *const contextp, Vgps_tb *const top);
void test_program_end();