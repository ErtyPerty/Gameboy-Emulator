#pragma once

#include "mapper.h"
#include <cstddef>
#include <cstdint>
#include <vector>

class MBC6 : public Mapper{
public:
    MBC6(uint8_t* rom, size_t rom_size);

    uint8_t read_rom(uint16_t addr) override;
    void write(uint16_t addr, uint8_t value) override;

    uint8_t read_ram(uint16_t addr) override;
    void write_ram(uint16_t addr, uint8_t value) override;

    void reset() override;

private:
    uint8_t read_flash(uint16_t addr, bool bank_a) const;

    uint8_t* rom;
    size_t rom_size;
    size_t rom_banks_8k;

    bool ram_enabled;
    bool flash_enabled;
    bool flash_write_enabled;

    uint8_t ram_bank_a;
    uint8_t ram_bank_b;
    uint8_t rom_bank_a;
    uint8_t rom_bank_b;

    bool flash_select_a;
    bool flash_select_b;

    std::vector<uint8_t> ram;
    std::vector<uint8_t> flash;
};
