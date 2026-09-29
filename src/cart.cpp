#include "cart.h"
#include "cart_type.h"
#include "memory_bus.h"
#include "serial_test.h"
#include "../include/MBC/no_mbc.h"
#include "../include/MBC/mbc1.h"

#include <memory>
#include <Windows.h>
#include <commdlg.h>
#include <iostream>
#include <fstream>
#include <cstdio>

// Declared here because memory_bus.h does not currently expose this function.
void memory_bus_set_mapper(Mapper* mapper);

char runtime_path_buffer[MAX_PATH];

uint8_t cartridge_data[MAX_CART_SIZE];
bool cartridge_loaded = false;

std::unique_ptr<Mapper> cartridge_mapper_instance;
size_t cartridge_size = 0;

cart_header_struct* cartridge_header =
    (cart_header_struct*)(cartridge_data + 0x100);

/* Helper to convert standard Game Boy RAM header codes to byte sizes */
static size_t get_ram_size_in_bytes(uint8_t ram_code) {
    switch (ram_code) {
        case 0x02: return 8 * 1024;    // 8 KB
        case 0x03: return 32 * 1024;   // 32 KB (4 banks of 8KB)
        case 0x04: return 128 * 1024;  // 128 KB (16 banks of 8KB)
        case 0x05: return 64 * 1024;   // 64 KB (8 banks of 8KB)
        default:   return 0;           // 0 KB / No RAM
    }
}

/* Uses Windows API to get file path */
bool get_runtime_path(){
    if (GetCurrentDirectoryA(MAX_PATH, runtime_path_buffer) == 0){
        std::cout << "Error getting current directory" << std::endl;
        return false;
    }

    return true;
}


/* Function to open a file dialog and load selected ROM Files */
bool cart_open_file(){
    if (!get_runtime_path()){
        return false;
    }

    char filename[MAX_PATH] = {};

    OPENFILENAMEA ofn;
    ZeroMemory(&ofn, sizeof(ofn));

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFilter = "GB ROM Files\0*.gb\0AnyFile\0*.*\0";
    ofn.lpstrFile = filename;
    ofn.lpstrInitialDir = runtime_path_buffer;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = "Select a Gameboy ROM File";
    ofn.Flags = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST;

    if (GetOpenFileNameA(&ofn)){
        return cart_load(filename);
    }

    std::cout << "Error opening file" << std::endl;
    return false;
}


/* Dumps cartridge header info into console */
void cart_print_info(){
    printf(
        "Entry Point: %.2X%.2X%.2X%.2X\n",
        cartridge_header->entry_point[0],
        cartridge_header->entry_point[1],
        cartridge_header->entry_point[2],
        cartridge_header->entry_point[3]
    );

    printf(
        "Title: %.15s\n",
        cartridge_header->title
    );

    printf(
        "CBG Flag : %.2X\n",
        cartridge_header->cbg_flag
    );

    printf(
        "New Licensee Code: %.2X%.2X\n",
        cartridge_header->new_licensee_code[0],
        cartridge_header->new_licensee_code[1]
    );

    printf(
        "SGB Flag: %.2X\n",
        cartridge_header->sgb_flag
    );

    printf(
        "Type: %.2X (%s)\n",
        cartridge_header->cartridge_type,
        cart_type_data[cartridge_header->cartridge_type].readable_name
    );

    printf(
        "ROM Size: %.2X\n",
        cartridge_header->rom_size
    );

    printf(
        "RAM Size: %.2X\n",
        cartridge_header->ram_size
    );

    printf(
        "Destination Code: %.2X\n",
        cartridge_header->destination_code
    );

    printf(
        "Old Licensee Code: %.2X\n",
        cartridge_header->old_licensee_code
    );

    printf(
        "Header Checksum: %.2X\n",
        cartridge_header->header_checksum
    );
}


bool cart_load(const char* filename){
    std::streampos size;
	serial_test_reset();

    // Clear the current cartridge state before loading another ROM.
    cartridge_loaded = false;
    cartridge_mapper_instance.reset();
    memory_bus_set_mapper(nullptr);
    cartridge_size = 0;

    std::ifstream file(
        filename,
        std::ios::in | std::ios::binary | std::ios::ate
    );

    if (!file.is_open()){
        std::cout << "Error loading file" << std::endl;
        return false;
    }

    size = file.tellg();

    if (size <= 0){
        std::cout << "Error: Cartridge is empty" << std::endl;
        file.close();
        return false;
    }

    if (size > MAX_CART_SIZE){
        std::cout << "Error: Cartridge size is too large" << std::endl;
        file.close();
        return false;
    }

    file.seekg(0, std::ios::beg);

    file.read(
        (char*)cartridge_data,
        static_cast<std::streamsize>(size)
    );

    if (!file){
        std::cout << "Error reading cartridge" << std::endl;
        file.close();
        return false;
    }

    file.close();

    cartridge_size = static_cast<size_t>(size);

    printf(
        "ROM %s loaded, size: %lli bytes\n",
        filename,
        (long long)cartridge_size
    );


    /*
     * Create the cartridge mapper.
     *
     * Cartridge types currently supported:
     *
     * 00 = ROM ONLY
     * 01 = MBC1
     * 02 = MBC1 + RAM
     * 03 = MBC1 + RAM + BATTERY
     * 08 = ROM + RAM
     * 09 = ROM + RAM + BATTERY
     */

    switch (cartridge_header->cartridge_type){

       /*
		* No MBC
		*/
		case 0x00:
		case 0x08:
		case 0x09:

			cartridge_mapper_instance =
				std::make_unique<class NoMBC>(
					&cartridge_data[0],
					cartridge_size
				);

			break;


		/*
		* MBC1
		*/
		case 0x01:
		case 0x02:
		case 0x03:

			cartridge_mapper_instance =
				std::make_unique<class MBC1>(
					&cartridge_data[0],
					cartridge_size,
					get_ram_size_in_bytes(cartridge_header->ram_size)
				);

			break;


        /*
         * Mapper not implemented yet
         */
        default:

            std::cout
                << "Unsupported cartridge type: 0x"
                << std::hex
                << static_cast<int>(
                    cartridge_header->cartridge_type
                )
                << std::dec
                << std::endl;

            cartridge_mapper_instance.reset();
            memory_bus_set_mapper(nullptr);

            return false;
    }


    /*
     * Give the memory bus access to the mapper.
     *
     * cart.cpp owns the mapper through unique_ptr.
     * The memory bus only stores a pointer to it.
     */
    memory_bus_set_mapper(
        cartridge_mapper_instance.get()
    );


    /*
     * Cartridge information/debug output.
     */
    printf(
        "Cartridge type: %02X\n",
        cartridge_header->cartridge_type
    );

    printf(
        "ROM size: %zu\n",
        cartridge_size
    );

    printf(
        "ROM direct [0000] = %02X\n",
        cartridge_data[0x0000]
    );

    printf(
        "ROM direct [0100] = %02X\n",
        cartridge_data[0x0100]
    );

    printf(
        "ROM bus    [0000] = %02X\n",
        memory_bus_read(0x0000)
    );

    printf(
        "ROM bus    [0100] = %02X\n",
        memory_bus_read(0x0100)
    );

    printf(
        "Mapper pointer = %p\n",
        (void*)cartridge_mapper_instance.get()
    );


    cartridge_loaded = true;

    return true;
}