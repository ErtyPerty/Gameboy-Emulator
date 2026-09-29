#include "huc3.h"
#include "mapper_utils.h"

static constexpr uint64_t GB_CPU_FREQUENCY = 4194304ULL;

HuC3::HuC3(uint8_t* rom, size_t rom_size)
    : rom(rom),
      rom_size(rom_size),
      rom_banks(mapper_rom_bank_count(rom, rom_size)),
      ram_size(mapper_ram_size_from_header(rom, rom_size)),
      rom_bank(0),
      ram_bank(0),
      mode(0),
      rtc_command(0),
      rtc_response(0),
      rtc_semaphore(1),
      rtc_address(0),
      rtc_cycles(0),
      ram(ram_size, 0xFF){
    rtc_memory.fill(0);
}

void HuC3::reset(){
    rom_bank = 0;
    ram_bank = 0;
    mode = 0;
    rtc_command = 0;
    rtc_response = 0;
    rtc_semaphore = 1;
    rtc_address = 0;
    rtc_cycles = 0;
}

void HuC3::tick(uint32_t cycles){
    rtc_cycles += cycles;

    while (rtc_cycles >= GB_CPU_FREQUENCY){
        rtc_cycles -= GB_CPU_FREQUENCY;

        // Keep a simple seconds counter in locations 00-02.
        uint32_t seconds =
            static_cast<uint32_t>(rtc_memory[0]) |
            (static_cast<uint32_t>(rtc_memory[1]) << 8) |
            (static_cast<uint32_t>(rtc_memory[2]) << 16);

        seconds++;

        rtc_memory[0] = static_cast<uint8_t>(seconds & 0xFF);
        rtc_memory[1] = static_cast<uint8_t>((seconds >> 8) & 0xFF);
        rtc_memory[2] = static_cast<uint8_t>((seconds >> 16) & 0xFF);
    }
}

uint8_t HuC3::read_rom(uint16_t addr){
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

void HuC3::write(uint16_t addr, uint8_t value){
    if (addr < 0x2000){
        mode = value & 0x0F;
        return;
    }

    if (addr < 0x4000){
        rom_bank = value & 0x7F;
        return;
    }

    if (addr < 0x6000){
        ram_bank = value & 0x03;
        return;
    }
}

uint8_t HuC3::read_ram(uint16_t addr){
    switch (mode){
        case 0xA:{
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

        case 0xC:
            return rtc_response & 0x0F;

        case 0xD:
            return rtc_semaphore;

        case 0xE:
            return 0xC0;

        default:
            return 0xFF;
    }
}

void HuC3::write_ram(uint16_t addr, uint8_t value){
    (void)addr;

    switch (mode){
        case 0xA:{
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
            return;
        }

        case 0xB:
            rtc_command = value & 0x0F;

            if ((rtc_command & 0x0F) == 0x01){
                rtc_response = rtc_memory[rtc_address];
                rtc_address++;
            }
            else if ((rtc_command & 0x0F) == 0x03){
                rtc_memory[rtc_address] = value & 0x0F;
                rtc_address++;
            }
            else if ((rtc_command & 0x0F) == 0x04){
                rtc_address = (rtc_address & 0xF0) | (value & 0x0F);
            }
            else if ((rtc_command & 0x0F) == 0x05){
                rtc_address = static_cast<uint8_t>(
                    (rtc_address & 0x0F) | ((value & 0x0F) << 4)
                );
            }

            return;

        case 0xD:
            if ((value & 0xFF) == 0xFE){
                rtc_semaphore = 1;
            }
            return;

        case 0xE:
            return;

        default:
            return;
    }
}
