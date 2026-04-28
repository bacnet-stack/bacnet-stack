# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
get_filename_component(basename ${CMAKE_CURRENT_SOURCE_DIR} NAME)

project(test_${basename}
    VERSION 1.0.0
    LANGUAGES C)

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
    )

include_directories(
    ${SRC_DIR}
    ${TST_DIR}/ztest/include
    )

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  set(PORT_BIP_SOURCE ${PORTS_DIR}/linux/bip-init.c)
  find_package(Threads REQUIRED)
elseif(WIN32)
  set(PORT_BIP_SOURCE ${PORTS_DIR}/win32/bip-init.c)
  add_compile_definitions(BACNET_PORT=win32)
elseif(APPLE)
  set(PORT_BIP_SOURCE ${PORTS_DIR}/bsd/bip-init.c)
else()
  message(FATAL_ERROR "bip_broadcast_port test is only supported on Linux, macOS, and Windows")
endif()

add_executable(${PROJECT_NAME}
  ${PORT_BIP_SOURCE}
  ${SRC_DIR}/bacnet/basic/sys/debug.c
  ./src/bvlc_stubs.c
  ./src/main.c
  ${ZTST_DIR}/ztest_mock.c
  ${ZTST_DIR}/ztest.c
  )

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  target_link_libraries(${PROJECT_NAME} Threads::Threads)
elseif(WIN32)
  target_link_libraries(${PROJECT_NAME} ws2_32)
endif()
