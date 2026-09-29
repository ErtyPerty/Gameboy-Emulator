#include "emulator_core.h"
#include "interrupts.h"
#include <cstdio>
#include <processenv.h>

#include "cart.h"
#include "cpu.h"
#include "timer.h"


uint32_t core_clock_counter = 0;
bool core_quit_requested = false;

int core_init(){ //handles opening the folder window and selecting a ROM file
    if (!cart_open_file()){
        return -1;
    }

    printf("SDL Initialized\n");

    cart_print_info();

    return 0;
}

void core_run(){
    cpu_reset();

    while (!core_quit_requested){
        cpu_tick();
    }
}

void core_shutdown(){
    
}

void core_advance_cpu_clocks(uint8_t clocks){
    timer_advance_clocks(clocks);
    core_clock_counter += clocks;

    if (interrupt_enable_ime_delay > 0){
        for (uint8_t c = 0; c < clocks; c++){
            interrupt_enable_ime_delay--;
            if (interrupt_enable_ime_delay == 0){
                interrupt_master_enable = true;
                break;
            }
        }
    }
}