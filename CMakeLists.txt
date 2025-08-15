cmake_minimum_required(VERSION 3.10)

project(tuya-mcu C)

set(CMAKE_C_STANDARD 99)

include_directories(src)

add_executable(tuya-mcu 
    src/main.c 
    src/tuya-mcu/tuya-mcu.c 
    src/tuya-mcu/tuya-dp.c
)
