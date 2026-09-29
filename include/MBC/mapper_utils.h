#pragma once

#include <cstddef>
#include <cstdint>

inline size_t mapper_ram_size_from_header(const uint8_t* rom, size_t rom_size){
    if (rom == nullptr || rom_size <= 0x149){
        return 0;
    }

    switch (rom[0x149]){
        case 0x00: return 0;
        case 0x01: return 2 * 1024;
        case 0x02: return 8 * 1024;
        case 0x03: return 32 * 1024;
        case 0x04: return 128 * 1024;
        case 0x05: return 64 * 1024;
        default:   return 0;
    }
}

inline size_t mapper_rom_size_from_header(const uint8_t* rom, size_t rom_size){
    if (rom == nullptr || rom_size <= 0x148){
        return rom_size;
    }

    const uint8_t code = rom[0x148];

    switch (code){
        case 0x00: return 32 * 1024;
        case 0x01: return 64 * 1024;
        case 0x02: return 128 * 1024;
        case 0x03: return 256 * 1024;
        case 0x04: return 512 * 1024;
        case 0x05: return 1024 * 1024;
        case 0x06: return 2 * 1024 * 1024;
        case 0x07: return 4 * 1024 * 1024;
        case 0x08: return 8 * 1024 * 1024;
        case 0x52: return 72 * 16 * 1024;
        case 0x53: return 80 * 16 * 1024;
        case 0x54: return 96 * 16 * 1024;
        default:   return rom_size;
    }
}

inline size_t mapper_rom_bank_count(const uint8_t* rom, size_t rom_size){
    const size_t size = mapper_rom_size_from_header(rom, rom_size);
    return (size + 0x3FFF) / 0x4000;
}

inline size_t mapper_wrap_rom_bank(size_t bank, size_t bank_count){
    if (bank_count == 0){
        return 0;
    }

    return bank % bank_count;
}

inline size_t mapper_wrap_ram_address(
    size_t address,
    size_t bank,
    size_t bank_size,
    size_t ram_size
){
    if (ram_size == 0 || bank_size == 0){
        return 0;
    }

    return ((address + (bank * bank_size)) % ram_size);
}
