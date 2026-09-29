#include "mbc5.h"
#include "mapper_utils.h"

#include <algorithm>

MBC5::MBC5(uint8_t* rom, size_t rom_size)
    : rom(rom),
      rom_size(rom_size),
      rom_banks(mapper_rom_bank_count(rom, rom_size)),
      ram_size(mapper_ram_size_from_header(rom, rom_size)),
      ram_enabled(false),
      rom_bank(0),
      ram_bank(0),
      rumble(false),
      ram(ram_size, 0xFF){
}

void MBC5::reset(){
    ram_enabled = false;
    rom_bank = 0;
    ram_bank = 0;
    rumble = false;
}

uint8_t MBC5::read_rom(uint16_t addr){
    if (addr < 0x4000){
        if (addr >= rom_size){
            return 0xFF;
        }

        return rom[addr];
    }

    const size_t bank = mapper_wrap_rom_bank(rom_bank, rom_banks);
    const size_t offset = (bank * 0x4000) + (addr - 0x4000);

    if (offset >= rom_size){
        return 0xFF;
    }

    return rom[offset];
}

void MBC5::write(uint16_t addr, uint8_t value){
    if (addr < 0x2000){
        ram_enabled = ((value & 0x0F) == 0x0A);
        return;
    }

    if (addr < 0x3000){
        rom_bank = static_cast<uint16_t>(
            (rom_bank & 0x100) | value
        );
        return;
    }

    if (addr < 0x4000){
        rom_bank = static_cast<uint16_t>(
            (rom_bank & 0x0FF) |
            ((value & 0x01) << 8)
        );
        return;
    }

    if (addr < 0x6000){
        if (value & 0x08){
            rumble = true;
        }
        else{
            rumble = false;
        }

        ram_bank = value & 0x0F;
        return;
    }
}

uint8_t MBC5::read_ram(uint16_t addr){
    if (!ram_enabled || ram_size == 0){
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

void MBC5::write_ram(uint16_t addr, uint8_t value){
    if (!ram_enabled || ram_size == 0){
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

bool MBC5::rumble_enabled() const{
    return rumble;
}
