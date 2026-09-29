#pragma once

#include "mapper.h"
#include <cstddef>
#include <cstdint>
#include <array>

class MBC2 : public Mapper{
public:
    MBC2(uint8_t* rom, size_t rom_size);

    uint8_t read_rom(uint16_t addr) override;
    void write(uint16_t addr, uint8_t value) override;

    uint8_t read_ram(uint16_t addr) override;
    void write_ram(uint16_t addr, uint8_t value) override;

    void reset() override;

private:
    uint8_t* rom;
    size_t rom_size;
    size_t rom_banks;

    bool ram_enabled;
    uint8_t rom_bank;

    std::array<uint8_t, 512> ram;
};
