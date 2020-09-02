# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)

list(APPEND BACNET_INCLUDE ../modules/lib/bacnet/src)

set(BACNET_FILE_BASENAME ai)
set(BACNET_FILE_SUBPATH bacnet/basic/object/${BACNET_FILE_BASENAME})
set(BACNET_BASE $ENV{ZEPHYR_BASE}/../modules/lib/bacnet)

list(APPEND BACNET_INCLUDE ../modules/lib/bacnet/src)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(${BACNET_FILE_BASENAME})

target_include_directories(app PRIVATE ${BACNET_INCLUDE})
target_sources(app PRIVATE
  ${BACNET_BASE}/test/${BACNET_FILE_SUBPATH}/src/main.c)
