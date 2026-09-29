#include "mbc6.h"
#include "mapper_utils.h"

#include <algorithm>

MBC6::MBC6(uint8_t* rom, size_t rom_size)
    : rom(rom),
      rom_size(rom_size),
      rom_banks_8k((mapper_rom_size_from_header(rom, rom_size) + 0x1FFF) / 0x2000),
      ram_enabled(false),
      flash_enabled(false),
      flash_write_enabled(false),
      ram_bank_a(0),
      ram_bank_b(0),
      rom_bank_a(0),
      rom_bank_b(0),
      flash_select_a(false),
      flash_select_b(false),
      ram(32 * 1024, 0xFF),
      flash(1024 * 1024, 0xFF){
}

void MBC6::reset(){
    ram_enabled = false;
    flash_enabled = false;
    flash_write_enabled = false;
    ram_bank_a = 0;
    ram_bank_b = 0;
    rom_bank_a = 0;
    rom_bank_b = 0;
    flash_select_a = false;
    flash_select_b = false;
}

uint8_t MBC6::read_flash(uint16_t addr, bool bank_a) const{
    const uint8_t bank = bank_a ? rom_bank_a : rom_bank_b;
    const size_t offset = (static_cast<size_t>(bank) * 0x2000) + (addr & 0x1FFF);

    if (offset >= flash.size()){
        return 0xFF;
    }

    return flash[offset];
}

uint8_t MBC6::read_rom(uint16_t addr){
    if (addr < 0x4000){
        if (addr >= rom_size){
            return 0xFF;
        }

        return rom[addr];
    }

    if (addr < 0x6000){
        if (flash_select_a && flash_enabled){
            return read_flash(addr, true);
        }

        const size_t bank = mapper_wrap_rom_bank(rom_bank_a, rom_banks_8k);
        const size_t offset = (bank * 0x2000) + (addr - 0x4000);

        if (offset >= rom_size){
            return 0xFF;
        }

        return rom[offset];
    }

    if (flash_select_b && flash_enabled){
        return read_flash(addr, false);
    }

    const size_t bank = mapper_wrap_rom_bank(rom_bank_b, rom_banks_8k);
    const size_t offset = (bank * 0x2000) + (addr - 0x6000);

    if (offset >= rom_size){
        return 0xFF;
    }

    return rom[offset];
}

void MBC6::write(uint16_t addr, uint8_t value){
    if (addr < 0x0400){
        ram_enabled = ((value & 0x0F) == 0x0A);
        return;
    }

    if (addr < 0x0800){
        ram_bank_a = value & 0x07;
        return;
    }

    if (addr < 0x0C00){
        ram_bank_b = value & 0x07;
        return;
    }

    if (addr < 0x1000){
        flash_enabled = (value & 0x01) != 0;
        return;
    }

    if (addr == 0x1000){
        flash_write_enabled = (value & 0x01) != 0;
        return;
    }

    if (addr < 0x2800){
        if (addr >= 0x2000){
            rom_bank_a = value & 0x7F;
        }
        return;
    }

    if (addr < 0x3000){
        flash_select_a = ((value & 0x08) != 0);
        return;
    }

    if (addr < 0x3800){
        rom_bank_b = value & 0x7F;
        return;
    }

    if (addr < 0x4000){
        flash_select_b = ((value & 0x08) != 0);
        return;
    }

    // Flash command execution is intentionally not emulated here yet.
    // Writes to the mapped flash area are ignored rather than modifying ROM.
    (void)value;
    (void)flash_write_enabled;
}

uint8_t MBC6::read_ram(uint16_t addr){
    if (!ram_enabled){
        return 0xFF;
    }

    size_t offset;

    if (addr < 0xB000){
        offset = static_cast<size_t>(ram_bank_a) * 0x1000 + (addr - 0xA000);
    }
    else{
        offset = static_cast<size_t>(ram_bank_b) * 0x1000 + (addr - 0xB000);
    }

    if (offset >= ram.size()){
        return 0xFF;
    }

    return ram[offset];
}

void MBC6::write_ram(uint16_t addr, uint8_t value){
    if (!ram_enabled){
        return;
    }

    size_t offset;

    if (addr < 0xB000){
        offset = static_cast<size_t>(ram_bank_a) * 0x1000 + (addr - 0xA000);
    }
    else{
        offset = static_cast<size_t>(ram_bank_b) * 0x1000 + (addr - 0xB000);
    }

    if (offset >= ram.size()){
        return;
    }

    ram[offset] = value;
}
