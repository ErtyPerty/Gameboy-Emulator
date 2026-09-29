#include "mbc3.h"
#include "mapper_utils.h"

#include <algorithm>

static constexpr uint64_t GB_CPU_FREQUENCY = 4194304ULL;

MBC3::MBC3(uint8_t* rom, size_t rom_size)
    : rom(rom),
      rom_size(rom_size),
      rom_banks(mapper_rom_bank_count(rom, rom_size)),
      ram_size(mapper_ram_size_from_header(rom, rom_size)),
      ram_enabled(false),
      rom_bank(1),
      ram_rtc_select(0),
      rtc_seconds(0),
      rtc_minutes(0),
      rtc_hours(0),
      rtc_days(0),
      rtc_halt(false),
      rtc_carry(false),
      latched_seconds(0),
      latched_minutes(0),
      latched_hours(0),
      latched_day_low(0),
      latched_day_high(0),
      latch_armed(false),
      rtc_cycles(0){
    ram.fill(0xFF);
}

void MBC3::reset(){
    ram_enabled = false;
    rom_bank = 1;
    ram_rtc_select = 0;
    latch_armed = false;
    rtc_cycles = 0;
}

void MBC3::update_rtc(uint32_t cycles){
    if (rtc_halt){
        return;
    }

    rtc_cycles += cycles;

    while (rtc_cycles >= GB_CPU_FREQUENCY){
        rtc_cycles -= GB_CPU_FREQUENCY;

        rtc_seconds++;

        if (rtc_seconds < 60){
            continue;
        }

        rtc_seconds = 0;
        rtc_minutes++;

        if (rtc_minutes < 60){
            continue;
        }

        rtc_minutes = 0;
        rtc_hours++;

        if (rtc_hours < 24){
            continue;
        }

        rtc_hours = 0;
        rtc_days++;

        if (rtc_days > 511){
            rtc_days = 0;
            rtc_carry = true;
        }
    }
}

void MBC3::tick(uint32_t cycles){
    update_rtc(cycles);
}

void MBC3::latch_rtc(){
    latched_seconds = rtc_seconds;
    latched_minutes = rtc_minutes;
    latched_hours = rtc_hours;
    latched_day_low = static_cast<uint8_t>(rtc_days & 0xFF);
    latched_day_high = static_cast<uint8_t>(
        ((rtc_days >> 8) & 0x01) |
        (rtc_halt ? 0x40 : 0x00) |
        (rtc_carry ? 0x80 : 0x00)
    );
}

uint8_t MBC3::read_rom(uint16_t addr){
    if (addr < 0x4000){
        if (addr >= rom_size){
            return 0xFF;
        }

        return rom[addr];
    }

    size_t bank = rom_bank & 0x7F;

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

void MBC3::write(uint16_t addr, uint8_t value){
    if (addr < 0x2000){
        ram_enabled = ((value & 0x0F) == 0x0A);
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
        ram_rtc_select = value;
        return;
    }

    if (addr < 0x8000){
        if (value == 0x00){
            latch_armed = true;
        }
        else if (value == 0x01 && latch_armed){
            latch_rtc();
            latch_armed = false;
        }

        return;
    }
}

uint8_t MBC3::read_ram(uint16_t addr){
    if (!ram_enabled){
        return 0xFF;
    }

    switch (ram_rtc_select){
        case 0x00:
        case 0x01:
        case 0x02:
        case 0x03:{
            if (ram_size == 0){
                return 0xFF;
            }

            const size_t index = mapper_wrap_ram_address(
                addr - 0xA000,
                ram_rtc_select,
                0x2000,
                ram_size
            );

            return ram[index];
        }

        case 0x08:
            return latched_seconds;

        case 0x09:
            return latched_minutes;

        case 0x0A:
            return latched_hours;

        case 0x0B:
            return latched_day_low;

        case 0x0C:
            return latched_day_high;

        default:
            return 0xFF;
    }
}

void MBC3::write_ram(uint16_t addr, uint8_t value){
    if (!ram_enabled){
        return;
    }

    switch (ram_rtc_select){
        case 0x00:
        case 0x01:
        case 0x02:
        case 0x03:{
            if (ram_size == 0){
                return;
            }

            const size_t index = mapper_wrap_ram_address(
                addr - 0xA000,
                ram_rtc_select,
                0x2000,
                ram_size
            );

            ram[index] = value;
            return;
        }

        case 0x08:
            rtc_seconds = value % 60;
            return;

        case 0x09:
            rtc_minutes = value % 60;
            return;

        case 0x0A:
            rtc_hours = value % 24;
            return;

        case 0x0B:
            rtc_days = static_cast<uint16_t>((rtc_days & 0x100) | value);
            return;

        case 0x0C:
            rtc_days = static_cast<uint16_t>(
                (rtc_days & 0xFF) |
                ((value & 0x01) << 8)
            );
            rtc_halt = (value & 0x40) != 0;
            rtc_carry = (value & 0x80) != 0;
            return;

        default:
            return;
    }
}
