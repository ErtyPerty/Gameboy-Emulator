#include "huc1.h"
#include "mapper_utils.h"

HuC1::HuC1(uint8_t* rom, size_t rom_size)
    : rom(rom),
      rom_size(rom_size),
      rom_banks(mapper_rom_bank_count(rom, rom_size)),
      ram_size(mapper_ram_size_from_header(rom, rom_size)),
      rom_bank(1),
      ram_bank(0),
      ir_mode(false),
      ram(ram_size, 0xFF),
      ir_value(0){
}

void HuC1::reset(){
    rom_bank = 1;
    ram_bank = 0;
    ir_mode = false;
    ir_value = 0;
}

uint8_t HuC1::read_rom(uint16_t addr){
    if (addr < 0x4000){
        if (addr >= rom_size){
            return 0xFF;
        }

        return rom[addr];
    }

    const size_t bank = mapper_wrap_rom_bank(rom_bank & 0x3F, rom_banks);
    const size_t offset = (bank * 0x4000) + (addr - 0x4000);

    if (offset >= rom_size){
        return 0xFF;
    }

    return rom[offset];
}

void HuC1::write(uint16_t addr, uint8_t value){
    if (addr < 0x2000){
        ir_mode = ((value & 0x0F) == 0x0E);
        return;
    }

    if (addr < 0x4000){
        rom_bank = value & 0x3F;
        if (rom_bank == 0){
            rom_bank = 1;
        }
        return;
    }

    if (addr < 0x6000){
        ram_bank = value & 0x03;
        return;
    }
}

uint8_t HuC1::read_ram(uint16_t addr){
    if (ir_mode){
        return static_cast<uint8_t>(0xC0 | (ir_value & 0x01));
    }

    if (ram_size == 0){
        return 0xFF;
    }

    const size_t index = mapper_wrap_ram_address(
        addr - 0xA000,
        ram_bank,
        0x2000,
        ram_size
    );

    return ram[index];
}

void HuC1::write_ram(uint16_t addr, uint8_t value){
    if (ir_mode){
        ir_value = value & 0x01;
        return;
    }

    if (ram_size == 0){
        return;
    }

    const size_t index = mapper_wrap_ram_address(
        addr - 0xA000,
        ram_bank,
        0x2000,
        ram_size
    );

    ram[index] = value;
}
