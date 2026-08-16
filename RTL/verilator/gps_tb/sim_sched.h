#pragma once

#include <cstdint>
#include <functional>

#include "verilated.h"

// Cooperative scheduler that lets the test program be written as straight-line
// blocking code while the simulation keeps advancing underneath it.
//
// The test program runs on its own OS thread, but the two threads never run at
// the same time: the sim thread parks while the test thread runs and vice
// versa, so only one thread ever touches the Verilated model.
//
// When the test thread blocks it hands the sim thread a wake predicate. The sim
// thread evaluates that inline every tick (cheap, no locking) and only performs
// the thread handoff once it becomes true, so handoffs cost one per event
// instead of one per tick.

// --- Sim thread side --------------------------------------------------------

// Spawn the test thread. It stays parked until the first sim_service() call.
void sim_start(VerilatedContext *const contextp, std::function<void()> body);

// Resume the test program if its wake predicate is satisfied. Call once per
// tick, after the device models have been ticked and before top->eval().
void sim_service();

// True once the test program body has returned.
bool sim_done();

// Unwind and join the test thread. Safe whether or not the body finished.
void sim_shutdown();

// --- Test thread side -------------------------------------------------------

// Block until wake() returns true. Always costs at least one tick, even if
// wake() is already true.
void sim_wait(std::function<bool()> wake);

// Block until the simulation reaches an absolute time / advances by n ticks.
void sim_wait_until_time(uint64_t time);
void sim_wait_ticks(uint64_t ticks);

// Current simulation time.
uint64_t sim_time();
