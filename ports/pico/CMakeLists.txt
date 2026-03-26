cmake_minimum_required(VERSION 3.13)

if (NOT DEFINED PICO_SDK_PATH AND DEFINED ENV{PICO_SDK_PATH})
    set(PICO_SDK_PATH "$ENV{PICO_SDK_PATH}")
endif()
if (DEFINED PICO_SDK_PATH)
    get_filename_component(
        PICO_SDK_PATH "${PICO_SDK_PATH}" REALPATH
        BASE_DIR "${CMAKE_CURRENT_BINARY_DIR}")
    set(ENV{PICO_SDK_PATH} "${PICO_SDK_PATH}")
endif()

# Default this port to Pico 2.
if (NOT PICO_BOARD)
    set(PICO_BOARD pico2 CACHE STRING "Pico SDK board for the BACnet Pico port" FORCE)
endif()
if (NOT DEFINED ENV{PICO_BOARD})
    set(ENV{PICO_BOARD} "${PICO_BOARD}")
endif()

include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(bacnet_pico_port C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

set(BACNET_ROOT ${CMAKE_CURRENT_LIST_DIR}/../..)
set(BACNET_SRC ${BACNET_ROOT}/src)
set(BACNET_CORE ${BACNET_SRC}/bacnet)
set(BACNET_BASIC ${BACNET_SRC}/bacnet/basic)

set(BACNET_COMMON_SOURCES
    ${BACNET_BASIC}/binding/address.c
    ${BACNET_BASIC}/object/client/device-client.c
    ${BACNET_BASIC}/npdu/h_npdu.c
    ${BACNET_BASIC}/service/h_apdu.c
    ${BACNET_BASIC}/service/h_cov.c
    ${BACNET_BASIC}/service/h_dcc.c
    ${BACNET_BASIC}/service/h_noserv.c
    ${BACNET_BASIC}/service/h_rd.c
    ${BACNET_BASIC}/service/h_rp.c
    ${BACNET_BASIC}/service/h_rpm.c
    ${BACNET_BASIC}/service/h_whohas.c
    ${BACNET_BASIC}/service/h_whois.c
    ${BACNET_BASIC}/service/h_wp.c
    ${BACNET_BASIC}/service/s_iam.c
    ${BACNET_BASIC}/service/s_ihave.c
    ${BACNET_BASIC}/sys/datetime_mstimer.c
    ${BACNET_BASIC}/sys/days.c
    ${BACNET_BASIC}/sys/dst.c
    ${BACNET_BASIC}/sys/debug.c
    ${BACNET_BASIC}/sys/ringbuf.c
    ${BACNET_BASIC}/sys/fifo.c
    ${BACNET_BASIC}/sys/mstimer.c
    ${BACNET_BASIC}/tsm/tsm.c
    ${BACNET_CORE}/abort.c
    ${BACNET_CORE}/bacaction.c
    ${BACNET_CORE}/bacaddr.c
    ${BACNET_CORE}/bacapp.c
    ${BACNET_CORE}/bacdcode.c
    ${BACNET_CORE}/bacdest.c
    ${BACNET_CORE}/bacdevobjpropref.c
    ${BACNET_CORE}/bacerror.c
    ${BACNET_CORE}/bacint.c
    ${BACNET_CORE}/bacreal.c
    ${BACNET_CORE}/bacstr.c
    ${BACNET_CORE}/datalink/cobs.c
    ${BACNET_CORE}/datalink/crc.c
    ${BACNET_CORE}/datalink/dlmstp.c
    ${BACNET_CORE}/datalink/mstp.c
    ${BACNET_CORE}/datalink/mstptext.c
    ${BACNET_CORE}/datetime.c
    ${BACNET_CORE}/dcc.c
    ${BACNET_CORE}/hostnport.c
    ${BACNET_CORE}/iam.c
    ${BACNET_CORE}/ihave.c
    ${BACNET_CORE}/indtext.c
    ${BACNET_CORE}/memcopy.c
    ${BACNET_CORE}/npdu.c
    ${BACNET_CORE}/proplist.c
    ${BACNET_CORE}/rd.c
    ${BACNET_CORE}/reject.c
    ${BACNET_CORE}/rp.c
    ${BACNET_CORE}/rpm.c
    ${BACNET_CORE}/timestamp.c
    ${BACNET_CORE}/whohas.c
    ${BACNET_CORE}/whois.c
    ${BACNET_CORE}/wp.c
)

set(BACNET_MSTP_SOURCES
    ${BACNET_COMMON_SOURCES}
    ${CMAKE_CURRENT_LIST_DIR}/dlenv.c
    ${CMAKE_CURRENT_LIST_DIR}/rs485.c
    ${CMAKE_CURRENT_LIST_DIR}/mstimer_init.c
    ${CMAKE_CURRENT_LIST_DIR}/main.c
)

add_executable(bacnet-pico-mstp ${BACNET_MSTP_SOURCES})

target_include_directories(bacnet-pico-mstp PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${BACNET_SRC}
)

target_compile_definitions(bacnet-pico-mstp PRIVATE
    BACDL_MSTP
    MAX_APDU=480
    BIG_ENDIAN=0
    PRINT_ENABLED=0
    MAX_TSM_TRANSACTIONS=1
    CRC_USE_TABLE
    BACAPP_MINIMAL
    BACAPP_TIMESTAMP
    BACNET_STACK_DEPRECATED_DISABLE
    BACNET_PROTOCOL_REVISION=16
)

target_link_libraries(bacnet-pico-mstp
    pico_stdlib
    hardware_uart
    hardware_gpio
    hardware_timer
)

pico_enable_stdio_usb(bacnet-pico-mstp 1)
pico_enable_stdio_uart(bacnet-pico-mstp 0)
pico_add_extra_outputs(bacnet-pico-mstp)
