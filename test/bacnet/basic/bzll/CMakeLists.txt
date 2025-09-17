# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.10 FATAL_ERROR)

get_filename_component(basename ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project(test_${basename}
    VERSION 1.0.0
    LANGUAGES C)

string(REGEX REPLACE
    "/test/bacnet/[a-zA-Z0-9_/-]*$"
    "/src"
    SRC_DIR
    ${CMAKE_CURRENT_SOURCE_DIR})
string(REGEX REPLACE
    "/test/bacnet/[a-zA-Z0-9_/-]*$"
    "/test"
    TST_DIR
    ${CMAKE_CURRENT_SOURCE_DIR})

add_compile_definitions(
    BIG_ENDIAN=0
    )

include_directories(
    ${SRC_DIR}
    )

add_executable(${PROJECT_NAME}
    # File(s) under test
    ${SRC_DIR}/bacnet/basic/bzll/bzllvmac.c
    # Support files and stubs (pathname alphabetical)
    ${SRC_DIR}/bacnet/bacaddr.c
    ${SRC_DIR}/bacnet/bacdcode.c
    ${SRC_DIR}/bacnet/bacint.c
    ${SRC_DIR}/bacnet/bacstr.c
    ${SRC_DIR}/bacnet/bacreal.c
    ${SRC_DIR}/bacnet/iam.c
    ${SRC_DIR}/bacnet/npdu.c
    ${SRC_DIR}/bacnet/basic/sys/bigend.c
    ${SRC_DIR}/bacnet/basic/sys/debug.c
    ${SRC_DIR}/bacnet/basic/sys/keylist.c
    ${SRC_DIR}/bacnet/hostnport.c
    # Test and test library files
    ./src/main.c
    )
