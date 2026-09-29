#pragma once

#include "mapper.h"

#include <cstddef>
#include <cstdint>
#include <vector>

class MBC1 : public Mapper{
public:
	MBC1(uint8_t* rom, size_t rom_size, size_t ram_size);

	uint8_t read_rom(uint16_t addr) override;
	void write(uint16_t addr, uint8_t value) override;

	uint8_t read_ram(uint16_t addr) override;
	void write_ram(uint16_t addr, uint8_t value) override;

	void reset() override;

private:
	uint8_t* rom;
	size_t rom_size;
	std::vector<uint8_t> ram;

	bool ram_enabled = false;
	bool ram_mode = false;

	uint8_t rom_bank_low = 1;
	uint8_t bank_high = 0;

	uint32_t get_lower_rom_bank() const;
	uint32_t get_upper_rom_bank() const;
	uint32_t get_ram_bank() const;
};
