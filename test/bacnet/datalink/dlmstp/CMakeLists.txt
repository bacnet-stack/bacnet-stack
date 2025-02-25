# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.10 FATAL_ERROR)

get_filename_component(basename ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project(test_${basename}
    VERSION 1.0.0
    LANGUAGES C)


string(REGEX REPLACE
    "/test/bacnet/[a-zA-Z_/-]*$"
    "/src"
    SRC_DIR
    ${CMAKE_CURRENT_SOURCE_DIR})
string(REGEX REPLACE
    "/test/bacnet/[a-zA-Z_/-]*$"
    "/test"
    TST_DIR
    ${CMAKE_CURRENT_SOURCE_DIR})
set(ZTST_DIR "${TST_DIR}/ztest/src")

add_compile_definitions(
    BIG_ENDIAN=0
    CONFIG_ZTEST=1
    )

include_directories(
    ${SRC_DIR}
    ${TST_DIR}/ztest/include
    ${TST_DIR}/bacnet/datalink/test
    )

add_executable(${PROJECT_NAME}
    # File(s) under test
    ${SRC_DIR}/bacnet/datalink/dlmstp.c
    # Support files and stubs (pathname alphabetical)
    ${SRC_DIR}/bacnet/bacaddr.c
    ${SRC_DIR}/bacnet/bacdcode.c
    ${SRC_DIR}/bacnet/bacint.c
    ${SRC_DIR}/bacnet/bacreal.c
    ${SRC_DIR}/bacnet/bacstr.c
    ${SRC_DIR}/bacnet/basic/sys/ringbuf.c
    ${SRC_DIR}/bacnet/basic/sys/days.c
    ${SRC_DIR}/bacnet/npdu.c
    # Test and test library files
    ./src/main.c
    ${TST_DIR}/bacnet/datalink/test/mstp-mock.c
    ${TST_DIR}/bacnet/datalink/test/mstp-rs485.c
    ${TST_DIR}/bacnet/basic/sys/test/mstimer_mock.c
    ${ZTST_DIR}/ztest_mock.c
    ${ZTST_DIR}/ztest.c
    )
