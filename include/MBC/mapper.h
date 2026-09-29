#pragma once

#include <cstdint>

class Mapper{
public:
	virtual ~Mapper() = default;

	virtual uint8_t read_rom(uint16_t addr) = 0;
	virtual void write(uint16_t addr, uint8_t value) = 0;

	virtual uint8_t read_ram(uint16_t addr) = 0;
	virtual void write_ram(uint16_t addr, uint8_t value) = 0;

	virtual void reset(){}
	virtual void tick(uint32_t cycles){}
};
