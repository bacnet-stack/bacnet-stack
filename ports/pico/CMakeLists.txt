# Example CMakeLists.txt for BACnet Stack Pico Port
cmake_minimum_required(VERSION 3.13)

# Include Pico SDK
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(bacnet_pico_port C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize the Pico SDK
pico_sdk_init()

set(BACNET_PROJECT_SOURCE
    ${CMAKE_SOURCE_DIR}/bip.c
    ${CMAKE_SOURCE_DIR}/bip-init.c
    ${CMAKE_SOURCE_DIR}/bvlc-pico.c
    ${CMAKE_SOURCE_DIR}/dlenv.c
    ${CMAKE_SOURCE_DIR}/rs485.c
    ${CMAKE_SOURCE_DIR}/mstimer_init.c
    # BACnet services library
    # BACnet core library
    )

add_executable(${CMAKE_PROJECT_NAME}
    ${BACNET_PROJECT_SOURCE}
)

target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC
    ${CMAKE_SOURCE_DIR}
    # BACnet Includes
)

target_link_libraries(${CMAKE_PROJECT_NAME}
    pico_stdlib
    hardware_uart
    hardware_gpio
)

pico_add_extra_outputs(${CMAKE_PROJECT_NAME})
pico_enable_stdio_usb(${CMAKE_PROJECT_NAME} 1)
pico_enable_stdio_uart(${CMAKE_PROJECT_NAME} 0)