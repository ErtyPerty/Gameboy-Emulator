cmake_minimum_required(VERSION 3.20)

project(my_emulator)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SDL3 REQUIRED)

add_executable(my_emulator
    "src/main.cpp"
    "src/cart.cpp"
)

target_include_directories(my_emulator PRIVATE
    "include"
)

target_link_libraries(my_emulator PRIVATE
    SDL3::SDL3
)