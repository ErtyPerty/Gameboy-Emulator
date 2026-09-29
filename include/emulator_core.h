//Initialises the emulator, runs the main loop and handles cleanup when closed

#pragma once
#include <stdint.h>

extern uint32_t core_clock_counter;

int core_init();
void core_run();
void core_shutdown();

//Advances the CPU clock counter. Sync between different hardware parts will be essential for accurate emulation.
void core_advance_cpu_clocks(uint8_t clocks);