cmake_minimum_required(VERSION 3.12)

# Pull in SDK (must be before project)
include(pico_sdk_import.cmake)

# VERSION 0.1.0
project(pico-cart-snes C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

if (PICO_SDK_VERSION_STRING VERSION_LESS "1.3.0")
    message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.3.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

# Initialize the SDK
pico_sdk_init()

include(CTest)
enable_testing()

add_compile_options(-Wall
        -Wno-format          # int != int32_t as far as the compiler is concerned because gcc has int32_t as long int
        -Wno-unused-function # we have some for the docs that aren't called
        -Wno-maybe-uninitialized
        )

add_executable(pico-cart-snes)

pico_generate_pio_header(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/psram.pio)

target_sources(${PROJECT_NAME} PRIVATE
        main.c
        logic_analyzer.c
        )

target_link_libraries(${PROJECT_NAME} PRIVATE
        pico_stdlib
        hardware_pio
        hardware_spi
        hardware_dma
        hardware_pwm
        )

pico_add_extra_outputs(${PROJECT_NAME})

pico_enable_stdio_uart(${PROJECT_NAME} 0) # DISABLE
pico_enable_stdio_usb(${PROJECT_NAME} 1)  # ENABLE

target_compile_options(${PROJECT_NAME} INTERFACE -DPICO_ENTER_USB_BOOT_ON_EXIT=1)

target_compile_definitions(${PROJECT_NAME} PUBLIC
        LOGIC_ANALYZER=1
        PICO_STDIO_USB_STDOUT_TIMEOUT_US=2000000000
        )

# re-configure stage2 boot to divide clock by 4 since we want to run at 288MHz
pico_define_boot_stage2(pico-div4 ${PICO_DEFAULT_BOOT_STAGE2_FILE})
target_compile_definitions(pico-div4 PRIVATE PICO_FLASH_SPI_CLKDIV=4)

pico_set_boot_stage2(${PROJECT_NAME} pico-div4)
