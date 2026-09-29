#pragma once

#include "mapper.h"
#include <cstddef>
#include <cstdint>
#include <array>
#include <vector>

class HuC3 : public Mapper{
public:
    HuC3(uint8_t* rom, size_t rom_size);

    uint8_t read_rom(uint16_t addr) override;
    void write(uint16_t addr, uint8_t value) override;

    uint8_t read_ram(uint16_t addr) override;
    void write_ram(uint16_t addr, uint8_t value) override;

    void reset() override;
    void tick(uint32_t cycles) override;

private:
    uint8_t* rom;
    size_t rom_size;
    size_t rom_banks;
    size_t ram_size;

    uint8_t rom_bank;
    uint8_t ram_bank;
    uint8_t mode;

    uint8_t rtc_command;
    uint8_t rtc_response;
    uint8_t rtc_semaphore;
    uint8_t rtc_address;

    std::array<uint8_t, 256> rtc_memory;
    std::vector<uint8_t> ram;

    uint64_t rtc_cycles;
};
