#pragma once

#include "mapper.h"
#include <cstddef>
#include <cstdint>
#include <array>

class MBC3 : public Mapper{
public:
    MBC3(uint8_t* rom, size_t rom_size);

    uint8_t read_rom(uint16_t addr) override;
    void write(uint16_t addr, uint8_t value) override;

    uint8_t read_ram(uint16_t addr) override;
    void write_ram(uint16_t addr, uint8_t value) override;

    void reset() override;
    void tick(uint32_t cycles) override;

private:
    void update_rtc(uint32_t cycles);
    void latch_rtc();

    uint8_t* rom;
    size_t rom_size;
    size_t rom_banks;
    size_t ram_size;

    bool ram_enabled;
    uint8_t rom_bank;
    uint8_t ram_rtc_select;

    std::array<uint8_t, 32 * 1024> ram;

    uint8_t rtc_seconds;
    uint8_t rtc_minutes;
    uint8_t rtc_hours;
    uint16_t rtc_days;
    bool rtc_halt;
    bool rtc_carry;

    uint8_t latched_seconds;
    uint8_t latched_minutes;
    uint8_t latched_hours;
    uint8_t latched_day_low;
    uint8_t latched_day_high;

    bool latch_armed;
    uint64_t rtc_cycles;
};
