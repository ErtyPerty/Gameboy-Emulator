#include "mbc7.h"
#include "mapper_utils.h"

MBC7::MBC7(uint8_t* rom, size_t rom_size)
    : rom(rom),
      rom_size(rom_size),
      rom_banks(mapper_rom_bank_count(rom, rom_size)),
      ram_enabled_1(false),
      ram_enabled_2(false),
      rom_bank(1),
      accel_x(0x81D0),
      accel_y(0x81D0),
      accel_latched(false){
    eeprom.fill(0xFF);
}

void MBC7::reset(){
    ram_enabled_1 = false;
    ram_enabled_2 = false;
    rom_bank = 1;
    accel_x = 0x81D0;
    accel_y = 0x81D0;
    accel_latched = false;
}

uint8_t MBC7::read_rom(uint16_t addr){
    if (addr < 0x4000){
        if (addr >= rom_size){
            return 0xFF;
        }

        return rom[addr];
    }

    const size_t bank = mapper_wrap_rom_bank(rom_bank & 0x7F, rom_banks);
    const size_t offset = (bank * 0x4000) + (addr - 0x4000);

    if (offset >= rom_size){
        return 0xFF;
    }

    return rom[offset];
}

void MBC7::write(uint16_t addr, uint8_t value){
    if (addr < 0x2000){
        ram_enabled_1 = ((value & 0x0F) == 0x0A);
        return;
    }

    if (addr < 0x4000){
        rom_bank = value & 0x7F;
        if (rom_bank == 0){
            rom_bank = 1;
        }
        return;
    }

    if (addr < 0x6000){
        ram_enabled_2 = (value == 0x40);
        return;
    }
}

uint8_t MBC7::read_ram(uint16_t addr){
    if (!ram_enabled_1 || !ram_enabled_2){
        return 0xFF;
    }

    const uint8_t reg = static_cast<uint8_t>((addr >> 4) & 0x0F);

    switch (reg){
        case 0x0: return accel_x & 0xFF;
        case 0x1: return accel_x >> 8;
        case 0x2: return accel_y & 0xFF;
        case 0x3: return accel_y >> 8;
        case 0x4: return 0x00;
        case 0x5: return 0xFF;
        case 0x8:
            return eeprom[addr & 0x00FF];
        default:
            return 0xFF;
    }
}

void MBC7::write_ram(uint16_t addr, uint8_t value){
    if (!ram_enabled_1 || !ram_enabled_2){
        return;
    }

    const uint8_t reg = static_cast<uint8_t>((addr >> 4) & 0x0F);

    if (reg == 0x0){
        if (value == 0x55){
            accel_x = 0x8000;
            accel_y = 0x8000;
            accel_latched = false;
        }
        return;
    }

    if (reg == 0x1){
        if (value == 0xAA){
            accel_x = 0x81D0;
            accel_y = 0x81D0;
            accel_latched = true;
        }
        return;
    }

    if (reg == 0x8){
        eeprom[addr & 0x00FF] = value;
    }
}
