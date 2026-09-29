#pragma once

#include "mapper.h"

#include <cstddef>
#include <cstdint>

class NoMBC : public Mapper{
public:
	NoMBC(uint8_t* rom, size_t rom_size);

	uint8_t read_rom(uint16_t addr) override;
	void write(uint16_t addr, uint8_t value) override;

	uint8_t read_ram(uint16_t addr) override;
	void write_ram(uint16_t addr, uint8_t value) override;

private:
	uint8_t* rom;
	size_t rom_size;
};
