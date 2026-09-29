#include "mbc1.h"

MBC1::MBC1(uint8_t* rom, size_t rom_size, size_t ram_size)
	: rom(rom),
	  rom_size(rom_size),
	  ram(ram_size, 0)
{
	reset();
}

void MBC1::reset(){
	ram_enabled = false;
	ram_mode = false;
	rom_bank_low = 1;
	bank_high = 0;
}

uint32_t MBC1::get_lower_rom_bank() const{
	if (!ram_mode){
		return 0;
	}

	const uint32_t bank = (bank_high & 0x03) << 5;
	const uint32_t bank_count = static_cast<uint32_t>(rom_size / 0x4000);

	if (bank_count == 0){
		return 0;
	}

	return bank % bank_count;
}

uint32_t MBC1::get_upper_rom_bank() const{
	uint32_t bank = rom_bank_low & 0x1F;

	if (bank == 0){
		bank = 1;
	}

	if (!ram_mode){
		bank |= (bank_high & 0x03) << 5;
	}

	const uint32_t bank_count = static_cast<uint32_t>(rom_size / 0x4000);

	if (bank_count <= 1){
		return 0;
	}

	bank %= bank_count;

	if (bank == 0){
		bank = 1;
	}

	return bank;
}

uint32_t MBC1::get_ram_bank() const{
	if (!ram_mode){
		return 0;
	}

	return bank_high & 0x03;
}

uint8_t MBC1::read_rom(uint16_t addr){
	if (addr <= 0x3FFF){
		const uint32_t bank = get_lower_rom_bank();
		const size_t offset = static_cast<size_t>(bank) * 0x4000 + addr;

		if (offset >= rom_size){
			return 0xFF;
		}

		return rom[offset];
	}

	if (addr <= 0x7FFF){
		const uint32_t bank = get_upper_rom_bank();
		const size_t offset = static_cast<size_t>(bank) * 0x4000 + (addr - 0x4000);

		if (offset >= rom_size){
			return 0xFF;
		}

		return rom[offset];
	}

	return 0xFF;
}

void MBC1::write(uint16_t addr, uint8_t value){
	if (addr >= 0x0000 && addr <= 0x1FFF){
		ram_enabled = (value & 0x0F) == 0x0A;
		return;
	}

	if (addr >= 0x2000 && addr <= 0x3FFF){
		rom_bank_low = value & 0x1F;

		if (rom_bank_low == 0){
			rom_bank_low = 1;
		}

		return;
	}

	if (addr >= 0x4000 && addr <= 0x5FFF){
		bank_high = value & 0x03;
		return;
	}

	if (addr >= 0x6000 && addr <= 0x7FFF){
		ram_mode = (value & 0x01) != 0;
	}
}

uint8_t MBC1::read_ram(uint16_t addr){
	if (!ram_enabled || ram.empty()){
		return 0xFF;
	}

	const uint32_t bank = get_ram_bank();
	const size_t offset = static_cast<size_t>(bank) * 0x2000 + (addr - 0xA000);

	return ram[offset % ram.size()];
}

void MBC1::write_ram(uint16_t addr, uint8_t value){
	if (!ram_enabled || ram.empty()){
		return;
	}

	const uint32_t bank = get_ram_bank();
	const size_t offset = static_cast<size_t>(bank) * 0x2000 + (addr - 0xA000);

	ram[offset % ram.size()] = value;
}
