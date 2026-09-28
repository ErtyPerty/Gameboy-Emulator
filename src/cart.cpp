#include "cart.h"
#include <iostream>
#include <fstream>
#include <cstdio>
#include <Windows.h>
#include <commdlg.h>

char runtime_path_buffer[MAX_PATH];
uint8_t cartridge_data[MAX_CART_SIZE];
bool cartridge_loaded = false;
cart_header_struct* cartridge_header = (cart_header_struct*)(cartridge_data + 0x100);

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

    char filename[MAX_PATH];

    OPENFILENAMEA ofn;
    ZeroMemory(&filename, sizeof(filename));
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
        cart_load(filename);
        return true;
    }
    else{
        std::cout << "Error opening file" << std::endl;
        return false;
    }
}

/* Dumps cartridge header info into console */
void cart_print_info(){
    printf("Entry Point: %.2X%.2X%.2X%.2X\n",
        cartridge_header->entry_point[0], 
        cartridge_header->entry_point[1], 
        cartridge_header->entry_point[2], 
        cartridge_header->entry_point[3]);

    printf("Title: %s\n", cartridge_header->title);
    printf("CBG Flag : %.2X\n", cartridge_header->cbg_flag);
    printf("New Licensee Code: %.2X%.2X\n", cartridge_header->new_licensee_code[0], cartridge_header->new_licensee_code[1]);
    printf("SGB Flag: %.2X\n", cartridge_header->sgb_flag);
    printf("Type: %.2X\n", cartridge_header->cartridge_type);
    printf("ROM Size: %.2X\n", cartridge_header->rom_size);
    printf("RAM Size: %.2X\n", cartridge_header->ram_size);
    printf("Destination Code: %.2X\n", cartridge_header->destination_code);
    printf("Old Licensee Code: %.2X\n", cartridge_header->old_licensee_code);
    printf("Header Checksum: %.2X\n", cartridge_header->header_checksum);
}

bool cart_load(const char* filename){
    std::streampos size;
    std::ifstream file(filename, std::ios::in | std::ios::binary | std::ios::ate);

    if (file.is_open()){
        size = file.tellg();
        if (size > MAX_CART_SIZE){
            std::cout << "Error: Cartridge size is too large" << std::endl;
            return false;
        }

        file.seekg(0, std::ios::beg);
        file.read((char*)cartridge_data, MAX_CART_SIZE);
        file.close();
        
        printf("ROM %s loaded, size: %lli bytes\n", filename, (long long)size);

        return true;
    }
    else{
        std::cout << "Error loading file" << std::endl;
        return false;
    }
}