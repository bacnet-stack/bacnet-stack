# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
get_filename_component(basename ${CMAKE_CURRENT_SOURCE_DIR} NAME)

project(test_${basename}
    VERSION 1.0.0
    LANGUAGES C)

find_package(Threads)

set(CMAKE_C_FLAGS -pthread)

string(REGEX REPLACE
    "/test/ports/[a-zA-Z_/-]*$"
    "/src"
    SRC_DIR
    ${CMAKE_CURRENT_SOURCE_DIR})
string(REGEX REPLACE
    "/test/ports/[a-zA-Z_/-]*$"
    "/ports"
    PORTS_DIR
    ${CMAKE_CURRENT_SOURCE_DIR})
string(REGEX REPLACE
    "/test/ports/[a-zA-Z_/-]*$"
    "/test"
    TST_DIR
    ${CMAKE_CURRENT_SOURCE_DIR})
set(ZTST_DIR "${TST_DIR}/ztest/src")

add_compile_definitions(
    BACNET_BIG_ENDIAN=0
    CONFIG_ZTEST=1
    BACNET_STACK_DEPRECATED_DISABLE=1
    _GNU_SOURCE
    )

include_directories(
    ${SRC_DIR}
    ${PORTS_DIR}/linux
    ${TST_DIR}/ztest/include
    )

add_executable(${PROJECT_NAME}
    # File(s) under test
    ${PORTS_DIR}/linux/dlmstp.c
    # Support files (pathname alphabetical)
    ${SRC_DIR}/bacnet/bacaddr.c
    ${SRC_DIR}/bacnet/bacdcode.c
    ${SRC_DIR}/bacnet/bacint.c
    ${SRC_DIR}/bacnet/bacreal.c
    ${SRC_DIR}/bacnet/bacstr.c
    ${SRC_DIR}/bacnet/bactext.c
    ${SRC_DIR}/bacnet/basic/sys/bigend.c
    ${SRC_DIR}/bacnet/basic/sys/debug.c
    ${SRC_DIR}/bacnet/basic/sys/fifo.c
    ${SRC_DIR}/bacnet/basic/sys/mstimer.c
    ${SRC_DIR}/bacnet/basic/sys/ringbuf.c
    ${SRC_DIR}/bacnet/datalink/cobs.c
    ${SRC_DIR}/bacnet/datalink/crc.c
    ${SRC_DIR}/bacnet/datalink/mstp.c
    ${SRC_DIR}/bacnet/datalink/mstptext.c
    ${SRC_DIR}/bacnet/indtext.c
    ${SRC_DIR}/bacnet/npdu.c
    ${PORTS_DIR}/linux/mstimer-init.c
    # Test and test library files
    ./src/rs485_stub.c
    ./src/main.c
    ${ZTST_DIR}/ztest_mock.c
    ${ZTST_DIR}/ztest.c
    )

target_link_libraries(${PROJECT_NAME} Threads::Threads)
