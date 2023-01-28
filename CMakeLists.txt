cmake_minimum_required(VERSION 3.12)

# Pull in SDK (must be before project)
include(pico_sdk_import.cmake)

project(rp2040 C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

if (PICO_SDK_VERSION_STRING VERSION_LESS "1.3.0")
    message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.3.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

set(PICO_EXAMPLES_PATH ${PROJECT_SOURCE_DIR})

# Initialize the SDK
pico_sdk_init()

include(example_auto_set_url.cmake)

add_compile_options(-Wall
        -Wno-format          # int != int32_t as far as the compiler is concerned because gcc has int32_t as long int
        -Wno-unused-function # we have some for the docs that aren't called
        -Wno-maybe-uninitialized
        -DPICO_MULTICORE
        )

if (PICO_ON_DEVICE)
    add_executable(rp2-weather
            src/main.c
            src/scheduler.c
            src/rtc_rp2040.c
            src/serial_rp2040.c
            src/error_rp2040.c
            src/i2c_rp2040.c
            src/spi_rp2040.c
            src/sensor.c
            src/nRF24L01.c
            src/heartbeat.c
            src/debug.c
            src/led_utils.c
            src/watchdog.c)

    # Disable SDK alarm support for this lowlevel example
    set(PICO_TIME_DEFAULT_ALARM_POOL_DISABLED 1)

    target_link_libraries(
            rp2-weather 
            pico_stdlib 
            pico_multicore 
            hardware_i2c 
            hardware_spi 
            hardware_uart)

    # create map/bin/hex file etc.
    pico_add_extra_outputs(rp2-weather)

    # add url via pico_set_program_url
    example_auto_set_url(rp2-weather)
endif ()