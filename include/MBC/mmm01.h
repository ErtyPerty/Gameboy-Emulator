#pragma once

#include "mapper.h"
#include <cstddef>
#include <cstdint>
#include <vector>

class MMM01 : public Mapper{
public:
    MMM01(uint8_t* rom, size_t rom_size);

    uint8_t read_rom(uint16_t addr) override;
    void write(uint16_t addr, uint8_t value) override;

    uint8_t read_ram(uint16_t addr) override;
    void write_ram(uint16_t addr, uint8_t value) override;

    void reset() override;

private:
    uint8_t* rom;
    size_t rom_size;
    size_t ram_size;

    bool mapped;
    bool multiplex;
    bool mbc1_mode;
    bool mode_write_locked;
    bool ram_enabled;

    uint8_t rom_bank_low;
    uint8_t rom_bank_mid;
    uint8_t rom_bank_high;
    uint8_t ram_bank_low;
    uint8_t ram_bank_high;
    uint8_t rom_bank_mask;
    uint8_t ram_bank_mask;

    std::vector<uint8_t> ram;

    size_t rom_bank_for_fixed_region() const;
    size_t rom_bank_for_switchable_region() const;
    size_t ram_bank_for_access() const;
};
