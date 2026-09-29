#include "timer.h"
#include "memory_bus.h"
#include "interrupts.h"

extern uint8_t cpu_halt_count;

const uint16_t timer_tac_edge_bits[4] = {9, 3, 5, 7};

gb_timer_registers* timer_registers = (gb_timer_registers*)(memory + 0xff04);

uint16_t timer_internal_sysclk = 0;
uint8_t timer_interrupt_delay = 0;

void timer_init(){
    timer_internal_sysclk = 0xABCC;
    timer_registers->timer_div = 0xAB;
    timer_registers->timer_tac = 0;
    timer_registers->timer_tima = 0;
    timer_registers->timer_tma = 0;
}

void timer_check_clock_edges(const uint16_t prev_sysclk){
    // Expose upper 8 bits of the internal divider.
    timer_registers->timer_div = timer_internal_sysclk >> 8;

    // Timer disabled.
    const bool tac_timer_enable =
        CHECK_BIT(timer_registers->timer_tac, 2);

    if (!tac_timer_enable){
        return;
    }

    const uint8_t tac_clock_select =
        timer_registers->timer_tac & 0b00000011;

    const uint16_t tac_divider_bit =
        timer_tac_edge_bits[tac_clock_select];

    const bool prev_edge =
        CHECK_BIT(prev_sysclk, tac_divider_bit);

    const bool curr_edge =
        CHECK_BIT(timer_internal_sysclk, tac_divider_bit);

    if (prev_edge && !curr_edge){
        timer_tick_tima();
    }
}

void timer_increase_div(const uint8_t cycles){
    for (uint8_t c = 0; c < cycles; c++){
        const uint16_t prev_sysclk = timer_internal_sysclk;
        timer_internal_sysclk++;

        timer_check_clock_edges(prev_sysclk);
    }
}

void timer_advance_clocks(const uint8_t cycles){
    if (cpu_halt_count == 2){
        return;
    }

    for (uint8_t c = 0; c < cycles; c++){
        if (timer_interrupt_delay > 0){
            timer_interrupt_delay--;

            if (timer_interrupt_delay == 0){
                timer_registers->timer_tima =
                    timer_registers->timer_tma;

                interrupt_raise_flag(INTERRUPT_FLAG_TIMER);
            }
        }

        timer_increase_div(1);
    }
}

void timer_on_div_write(const uint8_t _value){
    const uint16_t prev_sysclk = timer_internal_sysclk;
    timer_internal_sysclk = 0x0000;
    timer_check_clock_edges(prev_sysclk); //check foor falling edge bits, just to not clog APU and other bits
}

void timer_tick_tima(){
    if (timer_registers->timer_tima < 0xFF){
        timer_registers->timer_tima++;
        return;
    }

    timer_registers->timer_tima = 0x00;
    timer_interrupt_delay = 4;
}