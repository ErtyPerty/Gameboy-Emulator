#include "no_mbc.h"

NoMBC::NoMBC(uint8_t* rom, size_t rom_size)
	: rom(rom),
	  rom_size(rom_size)
{
}

uint8_t NoMBC::read_rom(uint16_t addr){
	if (addr >= rom_size){
		return 0xFF;
	}

	return rom[addr];
}

void NoMBC::write(uint16_t addr, uint8_t value){
	// No mapper registers.
}

uint8_t NoMBC::read_ram(uint16_t addr){
	return 0xFF;
}

void NoMBC::write_ram(uint16_t addr, uint8_t value){
	// No external RAM in the ROM-only configuration.
}
