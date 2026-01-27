# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.1)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.1.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(Sistema_Telemetria_DVI_RX C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()


# Add executable. Default name is the project name, version 0.1

add_executable(Sistema_Telemetria_DVI_RX 
    Sistema_Telemetria_DVI_RX.c 
    tmds_encode_font_2bpp.S
	tmds_encode_font_2bpp.h
    )

pico_set_program_name(Sistema_Telemetria_DVI_RX "Sistema_Telemetria_DVI_RX")
pico_set_program_version(Sistema_Telemetria_DVI_RX "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(Sistema_Telemetria_DVI_RX 1)
pico_enable_stdio_usb(Sistema_Telemetria_DVI_RX 1)

include_directories(${CMAKE_SOURCE_DIR}/libs/include) 

# Add the standard library to the build
target_link_libraries(Sistema_Telemetria_DVI_RX
        pico_stdlib
        )

target_compile_definitions(Sistema_Telemetria_DVI_RX PRIVATE
	DVI_VERTICAL_REPEAT=1
	)

# We have a lot in SRAM4 (particularly TMDS LUT) but don't need much stack on
# core 1. Probably even 256 bytes would be fine.
target_compile_definitions(Sistema_Telemetria_DVI_RX PRIVATE
	DVI_DEFAULT_SERIAL_CONFIG=${DVI_DEFAULT_SERIAL_CONFIG}
	PICO_CORE1_STACK_SIZE=0x200
	)

add_subdirectory(libdvi)
add_subdirectory(libsprite)

# Add any user requested libraries
target_link_libraries(Sistema_Telemetria_DVI_RX
	pico_stdlib
	pico_multicore
	libdvi
	hardware_adc
	hardware_i2c
	hardware_gpio
)

pico_add_extra_outputs(Sistema_Telemetria_DVI_RX)

