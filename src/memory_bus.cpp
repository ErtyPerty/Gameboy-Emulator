#include "memory_bus.h"
#include "../include/MBC/mapper.h"
#include "timer.h"
#include "interrupts.h"
#include "serial_test.h"

#include <stdio.h>

uint8_t memory[MEMORY_SIZE];
uint8_t eram[ERAM_SIZE];

// The currently active cartridge mapper.
// The cartridge system should assign this when loading a cartridge.
Mapper* cartridge_mapper = nullptr;


void memory_bus_set_mapper(Mapper* mapper){
    cartridge_mapper = mapper;
}


uint8_t memory_bus_read(const uint16_t addr){

    // Cartridge ROM
    if (addr >= 0x0000 && addr <= 0x7FFF){
        if (cartridge_mapper != nullptr){
            return cartridge_mapper->read_rom(addr);
        }

        return 0xFF;
    }

    // VRAM
    if (addr >= 0x8000 && addr <= 0x9FFF){
        // TODO: If PPU is in mode 3 the CPU cannot access VRAM
        return memory[addr];
    }

    // Cartridge RAM / cartridge registers
    if (addr >= 0xA000 && addr <= 0xBFFF){
        if (cartridge_mapper != nullptr){
            return cartridge_mapper->read_ram(addr);
        }

        return 0xFF;
    }

    // Work RAM 1
    if (addr >= 0xC000 && addr <= 0xCFFF){
        return memory[addr];
    }

    // Work RAM 2
    if (addr >= 0xD000 && addr <= 0xDFFF){
        return memory[addr];
    }

    // Echo RAM
    if (addr >= 0xE000 && addr <= 0xFDFF){
        return memory[addr - 0x2000];
    }

    // Object Attribute Memory
    if (addr >= 0xFE00 && addr <= 0xFE9F){
        // TODO: If PPU mode == 2 return 0xFF
        return memory[addr];
    }

    // Unusable memory
    if (addr >= 0xFEA0 && addr <= 0xFEFF){
        return 0xFF;
    }

    // I/O registers
    if (addr >= 0xFF00 && addr <= 0xFF7F){

        // Serial data
        if (addr == 0xFF01){
            return serial_test_read_data();
        }

        // Serial control
        if (addr == 0xFF02){
            return serial_test_read_control();
        }

        return memory[addr];
    }

    // High RAM
    if (addr >= 0xFF80 && addr <= 0xFFFE){
        return memory[addr];
    }

    // Interrupt Enable Register
    if (addr == 0xFFFF){
        return memory[addr];
    }

    return 0xFF;
}


void memory_bus_write(const uint16_t addr, const uint8_t value){

    // Cartridge mapper registers
    //
    // This covers all mapper writes in:
    // 0000-7FFF
    //
    // Examples:
    // MBC1 RAM enable / bank select
    // MBC2 RAM enable / ROM bank
    // MBC3 ROM / RAM / RTC selection
    // MBC5 ROM / RAM bank selection
    // etc.
    if (addr >= 0x0000 && addr <= 0x7FFF){
        if (cartridge_mapper != nullptr){
            cartridge_mapper->write(addr, value);
        }

        return;
    }

    // VRAM
    if (addr >= 0x8000 && addr <= 0x9FFF){
        // TODO: If PPU is in mode 3 the CPU cannot access VRAM
        memory[addr] = value;
        return;
    }

    // Cartridge RAM / cartridge registers
    //
    // For some cartridges this is normal RAM.
    // For others this may be RTC registers, EEPROM,
    // accelerometer registers, IR, etc.
    if (addr >= 0xA000 && addr <= 0xBFFF){
        if (cartridge_mapper != nullptr){
            cartridge_mapper->write_ram(addr, value);
        }

        return;
    }

    // Work RAM 1
    if (addr >= 0xC000 && addr <= 0xCFFF){
        memory[addr] = value;
        return;
    }

    // Work RAM 2
    if (addr >= 0xD000 && addr <= 0xDFFF){
        memory[addr] = value;
        return;
    }

    // Echo RAM
    if (addr >= 0xE000 && addr <= 0xFDFF){
        memory[addr - 0x2000] = value;
        return;
    }

    // Object Attribute Memory
    if (addr >= 0xFE00 && addr <= 0xFE9F){
        // TODO: If PPU mode is 2 or 3 the CPU cannot access OAM
        memory[addr] = value;
        return;
    }

    // Unusable memory
    if (addr >= 0xFEA0 && addr <= 0xFEFF){
        // All writes here are ignored
        return;
    }

    // I/O registers
    if (addr >= 0xFF00 && addr <= 0xFF7F){

        // Serial data
        if (addr == 0xFF01){
            serial_test_write_data(value);
            return;
        }

        // Serial control
        if (addr == 0xFF02){
            serial_test_write_control(value);
            return;
        }

        // Timer DIV
        if (addr == 0xFF04){
            timer_on_div_write(value);
            return;
        }

        // Timer control
        if (addr == 0xFF07){
            memory[addr] = value & 0b00000111;
            return;
        }

        // Interrupt Flag
        if (addr == 0xFF0F){
            interrupt_flag_write(value);
            return;
        }

        // OAM DMA
        if (addr == 0xFF46){
            const uint16_t source_addr = value * 0x100;
            const uint16_t destination_addr = 0xFE00;

            for (uint16_t i = 0; i < 160; i++){
                memory[destination_addr + i] =
                    memory_bus_read(source_addr + i);
            }

            return;
        }

        memory[addr] = value;
        return;
    }

    // High RAM
    if (addr >= 0xFF80 && addr <= 0xFFFE){
        memory[addr] = value;
        return;
    }

    // Interrupt Enable Register
    if (addr == 0xFFFF){
        interrupt_enable_write(value);
        return;
    }
}