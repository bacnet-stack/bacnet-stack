# Copyright (c) 2022 Legrand North America, LLC.
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

if(NOT ZEPHYR_CURRENT_MODULE_DIR)
  string(REGEX REPLACE "/zephyr/tests/[a-zA-Z_/-]*$" ""
    ZEPHYR_CURRENT_MODULE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
endif()

if(BOARD STREQUAL unit_testing)
  find_package(Zephyr COMPONENTS unittest REQUIRED HINTS $ENV{ZEPHYR_BASE})
  set(target testbinary)
else()
  find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
  set(target app)
endif()

project(bacnet_bacerror)
target_sources(${target} PRIVATE
  ${ZEPHYR_CURRENT_MODULE_DIR}/src/bacnet/bacerror.c
  ${ZEPHYR_CURRENT_MODULE_DIR}/test/unit/bacnet/bacerror/src/main.c
  ${ZEPHYR_CURRENT_MODULE_DIR}/test/unit/bacnet/bacerror/src/fakes/bacdcode.c
  ${CMAKE_CURRENT_SOURCE_DIR}/main.c
)


# NOTE for Zephyr >= v3.2.0:
# - Zephyr unittest builds for target 'testbinary' instead of 'app'.
# - Zephyr unittest does not generate ZEPHYR_<modulename>_MODULE_DIR.
# So we have to use relative paths to get to the source.

target_include_directories(${target} PRIVATE
  ${ZEPHYR_CURRENT_MODULE_DIR}/test/unit/bacnet/bacerror/src
  ${ZEPHYR_CURRENT_MODULE_DIR}/src
)
