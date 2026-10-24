#pragma once

#include <stdint.h>

void gps_init();

// Start and complete searches to start channels
void gps_task_search();

// Report status of active channels
void gps_task_report();

// Kill channels that have lost lock
void gps_task_kill();

// Process nav bits
void gps_task_decode();

// Solve for position
void gps_task_solve();

void epoch_callback(unsigned int gpio, uint32_t event_mask);
