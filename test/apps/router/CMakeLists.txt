# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
get_filename_component(basename ${CMAKE_CURRENT_SOURCE_DIR} NAME)

project(test_${basename}
    VERSION 1.0.0
    LANGUAGES C)

find_package(Threads)

set(CMAKE_C_FLAGS -pthread)

string(REGEX REPLACE
    "/test/apps/[a-zA-Z_/-]*$"
    "/src"
    SRC_DIR
    ${CMAKE_CURRENT_SOURCE_DIR})
string(REGEX REPLACE
    "/test/apps/[a-zA-Z_/-]*$"
    "/ports"
    PORTS_DIR
    ${CMAKE_CURRENT_SOURCE_DIR})
string(REGEX REPLACE
    "/test/apps/[a-zA-Z_/-]*$"
    "/apps"
    APPS_DIR
    ${CMAKE_CURRENT_SOURCE_DIR})
string(REGEX REPLACE
    "/test/apps/[a-zA-Z_/-]*$"
    "/test"
    TST_DIR
    ${CMAKE_CURRENT_SOURCE_DIR})
set(ZTST_DIR "${TST_DIR}/ztest/src")

# Production builds leave learned-route expiry disabled. Opt in here so
# the unit test still removes entries older than the configured age.
add_compile_definitions(
    BACNET_BIG_ENDIAN=0
    CONFIG_ZTEST=1
    BACNET_STACK_DEPRECATED_DISABLE=1
    _GNU_SOURCE
    ROUTER_DNET_MAX_AGE_MS=600000
    )

include_directories(
    ${SRC_DIR}
    ${APPS_DIR}/router
    ${PORTS_DIR}/linux
    ${TST_DIR}/ztest/include
    )

add_executable(${PROJECT_NAME}
    # File(s) under test
    ${APPS_DIR}/router/network_layer.c
    ${APPS_DIR}/router/portthread.c
    # Support files (pathname alphabetical)
    ${SRC_DIR}/bacnet/bacaddr.c
    ${SRC_DIR}/bacnet/bacdcode.c
    ${SRC_DIR}/bacnet/bacint.c
    ${SRC_DIR}/bacnet/bacreal.c
    ${SRC_DIR}/bacnet/bacstr.c
    ${SRC_DIR}/bacnet/bactext.c
    ${SRC_DIR}/bacnet/basic/sys/bigend.c
    ${SRC_DIR}/bacnet/basic/sys/debug.c
    ${SRC_DIR}/bacnet/indtext.c
    ${SRC_DIR}/bacnet/npdu.c
    ${PORTS_DIR}/linux/mstimer-init.c
    ${APPS_DIR}/router/msgqueue.c
    # Test and test library files
    ./src/main.c
    ${ZTST_DIR}/ztest_mock.c
    ${ZTST_DIR}/ztest.c
    )

target_link_libraries(${PROJECT_NAME} Threads::Threads)
