# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)

set(BACNET_FILE_BASENAME bacint)
set(BACNET_FILE_SUBPATH bacnet/${BACNET_FILE_BASENAME})
set(BACNET_BASE $ENV{ZEPHYR_BASE}/../modules/lib/bacnet)

list(APPEND BACNET_INCLUDE ../modules/lib/bacnet/src)

if(BOARD STREQUAL unit_testing)
  list(APPEND INCLUDE ${BACNET_INCLUDE})
  list(APPEND SOURCES
    ${BACNET_BASE}/src/${BACNET_FILE_SUBPATH}.c
    ${BACNET_BASE}/test/${BACNET_FILE_SUBPATH}/src/main.c)

  include($ENV{ZEPHYR_BASE}/subsys/testsuite/unittest.cmake)
  project(${BACNET_FILE_BASENAME})
else()
  include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
  project(${BACNET_FILE_BASENAME})

  target_include_directories(app PRIVATE ${BACNET_INCLUDE})
  target_sources(app PRIVATE
    ${BACNET_BASE}/test/${BACNET_FILE_SUBPATH}/src/main.c)
endif()
