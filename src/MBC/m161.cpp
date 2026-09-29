#include "m161.h"

M161::M161(uint8_t* rom, size_t rom_size)
	: rom(rom),
	  rom_size(rom_size)
{
	reset();
}

void M161::reset(){
}

uint8_t M161::read_rom(uint16_t addr){
	// TODO: Implement M161 banking behaviour.
	if (addr >= rom_size){
		return 0xFF;
	}

	return rom[addr];
}

void M161::write(uint16_t addr, uint8_t value){
	// TODO: Implement M161 mapper registers.
}

uint8_t M161::read_ram(uint16_t addr){
	// TODO: Implement M161 RAM / RTC / peripheral reads.
	return 0xFF;
}

void M161::write_ram(uint16_t addr, uint8_t value){
	// TODO: Implement M161 RAM / RTC / peripheral writes.
}
