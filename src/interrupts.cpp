#include "interrupts.h"
#include "memory_bus.h"
#include "emulator_core.h"
#include "cpu.h"
#include <cstdio>

extern uint8_t cpu_halt_count;

bool interrupt_master_enable = false;
uint8_t interrupt_enable_ime_delay = 0;
bool interrupt_stat_signal = false;

void interrupt_enable_write(uint8_t value)
{
    memory[ADDR_IO_IE] = value;
}

void interrupt_flag_write(uint8_t value)
{
    memory[ADDR_IO_IF] = value | 0xE0; // Bits 5-7 always read as 1
}

void interrupt_raise_flag(uint8_t flag){
    memory[ADDR_IO_IF] |= flag;
}

void interrupt_jump_to(uint16_t addr)
{

    printf(
        "\n[INTERRUPT] PC=%04X SP=%04X IF=%02X IE=%02X IME=%d DELAY=%d\n",
        cpu_registers.pc,
        cpu_registers.sp,
        memory[ADDR_IO_IF],
        memory[ADDR_IO_IE],
        interrupt_master_enable,
        interrupt_enable_ime_delay
    );

    interrupt_master_enable = false; // Disable IME immediately upon handling

    const uint8_t pchi = (cpu_registers.pc & 0xFF00) >> 8;
    const uint8_t pclo = (cpu_registers.pc & 0xFF);

    core_advance_cpu_clocks(4);
    
    cpu_registers.sp--;
    memory_bus_write(cpu_registers.sp, pchi); // Ensure memory_bus_write timing aligns

    cpu_registers.sp--;
    memory_bus_write(cpu_registers.sp, pclo);

    printf(
        "[INTERRUPT] Saved PC=%04X at SP=%04X/%04X\n",
        cpu_registers.pc,
        cpu_registers.sp,
        cpu_registers.sp + 1
    );

    core_advance_cpu_clocks(4);
    cpu_registers.pc = addr;
    core_advance_cpu_clocks(4);
}

void interrupt_service_routine(){
    const uint8_t interrupt_enable = memory[ADDR_IO_IE];
    const uint8_t interrupt_flag = memory[ADDR_IO_IF];

    const uint8_t pending =
        interrupt_enable &
        interrupt_flag &
        0x1F;

    if (pending == 0){
        return;
    }

    if (cpu_halt_count == 1){
        core_advance_cpu_clocks(4);
        cpu_halt_count = 0;
    }

    if (!interrupt_master_enable){
        return;
    }

    if (pending & INTERRUPT_FLAG_VBLANK){
        CLEAR_BIT(memory[ADDR_IO_IF], 0);
        interrupt_jump_to(INTERRUPT_HANDLER_V_BLANK);
    }
    else if (pending & INTERRUPT_FLAG_STAT){
        CLEAR_BIT(memory[ADDR_IO_IF], 1);
        interrupt_jump_to(INTERRUPT_HANDLER_LCD_STATUS);
    }
    else if (pending & INTERRUPT_FLAG_TIMER){
        CLEAR_BIT(memory[ADDR_IO_IF], 2);
        interrupt_jump_to(INTERRUPT_HANDLER_TIMER);
    }
    else if (pending & INTERRUPT_FLAG_SERIAL){
        CLEAR_BIT(memory[ADDR_IO_IF], 3);
        interrupt_jump_to(INTERRUPT_HANDLER_SERIAL);
    }
    else if (pending & INTERRUPT_FLAG_JOYPAD){
        CLEAR_BIT(memory[ADDR_IO_IF], 4);
        interrupt_jump_to(INTERRUPT_HANDLER_JOYPAD);
    }
}