#pragma once

#include "mapper.h"
#include <cstddef>
#include <cstdint>
#include <vector>

class HuC1 : public Mapper{
public:
    HuC1(uint8_t* rom, size_t rom_size);

    uint8_t read_rom(uint16_t addr) override;
    void write(uint16_t addr, uint8_t value) override;

    uint8_t read_ram(uint16_t addr) override;
    void write_ram(uint16_t addr, uint8_t value) override;

    void reset() override;

private:
    uint8_t* rom;
    size_t rom_size;
    size_t rom_banks;
    size_t ram_size;

    uint8_t rom_bank;
    uint8_t ram_bank;
    bool ir_mode;

    std::vector<uint8_t> ram;
    uint8_t ir_value;
};
