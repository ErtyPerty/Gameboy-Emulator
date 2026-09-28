#include <iostream>
#include <cstdio>
#include <Windows.h>

#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "cart.h"

int SDLCALL emulator_runapp_callback(int argc, char* argv[]);

int main(int argc, char* argv[]){
    return SDL_RunApp(argc, argv, emulator_runapp_callback, NULL);
}

int SDLCALL emulator_runapp_callback(int argc, char* argv[]){
    if (!SDL_Init(SDL_INIT_VIDEO)){
        return -1;
    }

    char current_directory[MAX_PATH];

    if (GetCurrentDirectoryA(MAX_PATH, current_directory)){
        printf("Current Directory: %s\n", current_directory);
    }

    const char* pokemon_path = "roms/Pokemon - Blue Version.gb";

    printf("Trying to load: %s\n", pokemon_path);

    if (cart_load(pokemon_path)){
        cart_print_info();
    }

    printf("SDL Initialized\n");

    SDL_Quit();

    return 0;
}