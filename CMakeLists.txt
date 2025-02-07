# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.0)
set(toolchainVersion 13_3_Rel1)
set(picotoolVersion 2.1.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
# set(PICO_BOARD pico CACHE STRING "Board type")

cmake_minimum_required(VERSION 3.29)

# Pull in SDK (must be before project)
include(pico_sdk_import.cmake)
include(pico_extras_import_optional.cmake)

project(rp2-weather C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

if (PICO_SDK_VERSION_STRING VERSION_LESS "2.1.0")
    message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.3.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

set(PICO_EXAMPLES_PATH ${PROJECT_SOURCE_DIR})

# Initialize the SDK
pico_sdk_init()

include(example_auto_set_url.cmake)

# pico_generate_pio_header(pulsecount ${CMAKE_CURRENT_LIST_DIR}/pio/pulsecount.pio)

add_compile_options(
        -Wall
        -Wno-format
        -Wno-unused-function
        -Wno-maybe-uninitialized
        )

add_executable(
        rp2-weather
        src/main.c
        src/scheduler.c
        src/rtc_rp2040.c
        src/serial_rp2040.c
        src/error_rp2040.c
        src/i2c_rp2040.c
        src/spi_rp2040.c
        src/pio_rp2040.c
        src/pwm_rp2040.c
        src/logger.c
        src/packet.c
        src/sensor.c
        src/TMP117.c
        src/SHT4X.c
        src/icp10125.c
        src/max17048.c
        src/nRF24L01.c
        src/heartbeat.c
        src/battery.c
        src/utils.c
        src/watchdog.c
        src/gpio_cntrl.c)

    # # Disable SDK alarm support for this lowlevel example
    set(PICO_TIME_DEFAULT_ALARM_POOL_DISABLED 1)

    target_link_libraries(
            rp2-weather 
            pico_stdlib 
            pico_multicore 
            hardware_i2c 
            hardware_spi 
            hardware_uart
            hardware_pio
            hardware_adc
            hardware_pwm
            hardware_rtc
            hardware_clocks)

    # create map/bin/hex file etc.
    pico_add_extra_outputs(rp2-weather)

    # add url via pico_set_program_url
    example_auto_set_url(rp2-weather)
