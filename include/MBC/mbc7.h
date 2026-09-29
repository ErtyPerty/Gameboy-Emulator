#pragma once

#include "mapper.h"
#include <cstddef>
#include <cstdint>
#include <array>

class MBC7 : public Mapper{
public:
    MBC7(uint8_t* rom, size_t rom_size);

    uint8_t read_rom(uint16_t addr) override;
    void write(uint16_t addr, uint8_t value) override;

    uint8_t read_ram(uint16_t addr) override;
    void write_ram(uint16_t addr, uint8_t value) override;

    void reset() override;

private:
    uint8_t* rom;
    size_t rom_size;
    size_t rom_banks;

    bool ram_enabled_1;
    bool ram_enabled_2;
    uint8_t rom_bank;

    uint16_t accel_x;
    uint16_t accel_y;
    bool accel_latched;
    std::array<uint8_t, 256> eeprom;
};
