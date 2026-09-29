#include "mmm01.h"
#include "mapper_utils.h"

MMM01::MMM01(uint8_t* rom, size_t rom_size)
    : rom(rom),
      rom_size(rom_size),
      ram_size(mapper_ram_size_from_header(rom, rom_size)),
      mapped(false),
      multiplex(false),
      mbc1_mode(false),
      mode_write_locked(false),
      ram_enabled(false),
      rom_bank_low(1),
      rom_bank_mid(0),
      rom_bank_high(0),
      ram_bank_low(0),
      ram_bank_high(0),
      rom_bank_mask(0),
      ram_bank_mask(0),
      ram(ram_size, 0xFF){
}

void MMM01::reset(){
    mapped = false;
    multiplex = false;
    mbc1_mode = false;
    mode_write_locked = false;
    ram_enabled = false;
    rom_bank_low = 1;
    rom_bank_mid = 0;
    rom_bank_high = 0;
    ram_bank_low = 0;
    ram_bank_high = 0;
    rom_bank_mask = 0;
    ram_bank_mask = 0;
}

size_t MMM01::rom_bank_for_fixed_region() const{
    if (!mapped){
        const size_t bank_count = rom_size / 0x4000;

        if (bank_count < 2){
            return 0;
        }

        // Startup/unmapped mode maps the final 32 KiB to the CPU address space.
        return (bank_count >= 2) ? (bank_count - 2) : 0;
    }

    if (multiplex && mbc1_mode){
        const size_t low = ram_bank_low & 0x03;
        return (
            (static_cast<size_t>(rom_bank_high & 0x03) << 7) |
            (low << 5) |
            (rom_bank_low & 0x1F)
        );
    }

    if (mbc1_mode){
        return static_cast<size_t>(rom_bank_high & 0x03) << 5;
    }

    return 0;
}

size_t MMM01::rom_bank_for_switchable_region() const{
    if (!mapped){
        const size_t bank_count = rom_size / 0x4000;

        if (bank_count == 0){
            return 0;
        }

        return bank_count - 1;
    }

    size_t bank = 0;

    if (multiplex){
        bank |= static_cast<size_t>(rom_bank_high & 0x03) << 7;
        bank |= static_cast<size_t>(ram_bank_low & 0x03) << 5;
    }
    else{
        bank |= static_cast<size_t>(rom_bank_high & 0x03) << 5;
    }

    const uint8_t masked_low =
        rom_bank_low & static_cast<uint8_t>(~rom_bank_mask);

    bank |= masked_low & 0x1F;

    if ((masked_low & 0x1F) == 0){
        bank |= 0x01;
    }

    return bank;
}

size_t MMM01::ram_bank_for_access() const{
    if (multiplex){
        return static_cast<size_t>(ram_bank_high & 0x03) | (
            static_cast<size_t>(rom_bank_mid & 0x03) << 2
        );
    }

    if (!mbc1_mode){
        return 0;
    }

    return static_cast<size_t>(ram_bank_high & 0x03) | (
        static_cast<size_t>(ram_bank_low & 0x03) << 2
    );
}

uint8_t MMM01::read_rom(uint16_t addr){
    size_t bank;

    if (addr < 0x4000){
        bank = rom_bank_for_fixed_region();
    }
    else{
        bank = rom_bank_for_switchable_region();
    }

    const size_t offset = (bank * 0x4000) + (addr & 0x3FFF);

    if (offset >= rom_size){
        return 0xFF;
    }

    return rom[offset];
}

void MMM01::write(uint16_t addr, uint8_t value){
    if (addr < 0x2000){
        ram_enabled = ((value & 0x0F) == 0x0A);

        if (!mapped){
            ram_bank_mask = (value >> 4) & 0x03;
        }

        if (!mapped && (value & 0x40)){
            mapped = true;
        }

        return;
    }

    if (addr < 0x4000){
        const uint8_t writable = static_cast<uint8_t>(~rom_bank_mask);
        const uint8_t low = value & 0x1F;

        rom_bank_low = static_cast<uint8_t>(
            (rom_bank_low & rom_bank_mask) |
            (low & writable)
        );

        if (!mapped){
            rom_bank_mid = (value >> 5) & 0x03;
        }

        return;
    }

    if (addr < 0x6000){
        ram_bank_low = value & 0x03;

        if (!mapped){
            ram_bank_high = (value >> 2) & 0x03;
            rom_bank_high = (value >> 4) & 0x03;
            mode_write_locked = (value & 0x40) != 0;
        }

        return;
    }

    if (addr < 0x8000){
        if (mapped && mode_write_locked){
            return;
        }

        mbc1_mode = (value & 0x01) != 0;

        if (!mapped){
            rom_bank_mask = (value >> 1) & 0x1F;
            rom_bank_mask &= 0x1E;
            multiplex = (value & 0x40) != 0;
        }

        return;
    }
}

uint8_t MMM01::read_ram(uint16_t addr){
    if (!ram_enabled || ram_size == 0){
        return 0xFF;
    }

    const size_t bank = ram_bank_for_access();
    const size_t offset = mapper_wrap_ram_address(
        addr - 0xA000,
        bank,
        0x2000,
        ram_size
    );

    return ram[offset];
}

void MMM01::write_ram(uint16_t addr, uint8_t value){
    if (!ram_enabled || ram_size == 0){
        return;
    }

    const size_t bank = ram_bank_for_access();
    const size_t offset = mapper_wrap_ram_address(
        addr - 0xA000,
        bank,
        0x2000,
        ram_size
    );

    ram[offset] = value;
}
