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
    )

add_executable(${PROJECT_NAME}
    # File(s) under test
    ${SRC_DIR}/bacnet/basic/object/osv.c
    # Support files and stubs (pathname alphabetical)
    ${SRC_DIR}/bacnet/access_rule.c
    ${SRC_DIR}/bacnet/bacaction.c
    ${SRC_DIR}/bacnet/bacaddr.c
    ${SRC_DIR}/bacnet/bacapp.c
    ${SRC_DIR}/bacnet/bacdcode.c
    ${SRC_DIR}/bacnet/bacdest.c
    ${SRC_DIR}/bacnet/bacdevobjpropref.c
    ${SRC_DIR}/bacnet/bacint.c
    ${SRC_DIR}/bacnet/baclog.c
    ${SRC_DIR}/bacnet/bacreal.c
    ${SRC_DIR}/bacnet/bacstr.c
    ${SRC_DIR}/bacnet/bactext.c
    ${SRC_DIR}/bacnet/basic/sys/bigend.c
    ${SRC_DIR}/bacnet/datetime.c
    ${SRC_DIR}/bacnet/basic/sys/days.c
    ${SRC_DIR}/bacnet/indtext.c
    ${SRC_DIR}/bacnet/hostnport.c
    ${SRC_DIR}/bacnet/lighting.c
    ${SRC_DIR}/bacnet/proplist.c
    ${SRC_DIR}/bacnet/timer_value.c
    ${SRC_DIR}/bacnet/timestamp.c
    ${SRC_DIR}/bacnet/wp.c
    ${SRC_DIR}/bacnet/weeklyschedule.c
    ${SRC_DIR}/bacnet/bactimevalue.c
    ${SRC_DIR}/bacnet/dailyschedule.c
    ${SRC_DIR}/bacnet/calendar_entry.c
    ${SRC_DIR}/bacnet/special_event.c
    ${SRC_DIR}/bacnet/channel_value.c
    ${SRC_DIR}/bacnet/secure_connect.c
    # Test and test library files
    ./src/main.c
    ${ZTST_DIR}/ztest_mock.c
    ${ZTST_DIR}/ztest.c
    )
