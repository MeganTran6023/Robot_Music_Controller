cmake_minimum_required(VERSION 3.13)

# Pico W board must be set before pico_sdk_init()[cite: 5].
set(PICO_BOARD pico_w)

# Add these lines to print the environment variables[cite: 4, 5]:
message(STATUS "--- DEBUG ENVIRONMENT VARIABLES ---")
message(STATUS "PICO_SDK_PATH: '$ENV{PICO_SDK_PATH}'")
message(STATUS "FREERTOS_KERNEL_PATH: '$ENV{FREERTOS_KERNEL_PATH}'")
message(STATUS "-----------------------------------")

# Use your environment variables instead of hardcoded relative paths[cite: 4, 5]
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)
include($ENV{FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2040/FreeRTOS_Kernel_import.cmake)

project(blink_project)

pico_sdk_init()

add_executable(main main.c)

target_include_directories(main PRIVATE ${CMAKE_CURRENT_LIST_DIR})

# Link the standard library, FreeRTOS, the Pico W wireless/LED architecture, ADC[cite: 4], and hardware_i2c[cite: 5].
target_link_libraries(main
    pico_stdlib
    FreeRTOS-Kernel-Heap4
    pico_cyw43_arch_none
    hardware_adc
    hardware_i2c
)

pico_enable_stdio_usb(main 1)
pico_enable_stdio_uart(main 0)
pico_add_extra_outputs(main)