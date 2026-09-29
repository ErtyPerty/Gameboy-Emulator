#include "mbc2.h"
#include "mapper_utils.h"

#include <algorithm>

MBC2::MBC2(uint8_t* rom, size_t rom_size)
    : rom(rom),
      rom_size(rom_size),
      rom_banks(mapper_rom_bank_count(rom, rom_size)),
      ram_enabled(false),
      rom_bank(1){
    ram.fill(0x0F);
}

void MBC2::reset(){
    ram_enabled = false;
    rom_bank = 1;
}

uint8_t MBC2::read_rom(uint16_t addr){
    if (addr < 0x4000){
        if (addr >= rom_size){
            return 0xFF;
        }

        return rom[addr];
    }

    size_t bank = rom_bank & 0x0F;
    if (bank == 0){
        bank = 1;
    }

    bank = mapper_wrap_rom_bank(bank, rom_banks);

    const size_t offset = (bank * 0x4000) + (addr - 0x4000);

    if (offset >= rom_size){
        return 0xFF;
    }

    return rom[offset];
}

void MBC2::write(uint16_t addr, uint8_t value){
    if (addr < 0x2000){
        if ((addr & 0x0100) == 0){
            ram_enabled = ((value & 0x0F) == 0x0A);
        }

        return;
    }

    if (addr < 0x4000){
        if ((addr & 0x0100) != 0){
            rom_bank = value & 0x0F;

            if (rom_bank == 0){
                rom_bank = 1;
            }
        }

        return;
    }
}

uint8_t MBC2::read_ram(uint16_t addr){
    if (!ram_enabled){
        return 0xFF;
    }

    const size_t index = addr & 0x01FF;
    return static_cast<uint8_t>(0xF0 | (ram[index] & 0x0F));
}

void MBC2::write_ram(uint16_t addr, uint8_t value){
    if (!ram_enabled){
        return;
    }

    ram[addr & 0x01FF] = value & 0x0F;
}
