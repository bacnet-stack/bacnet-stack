# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.13.1)

# Extract module path and names
string(REGEX REPLACE
  "/zephyr/tests/[a-zA-Z_/-]*$" ""
  BACNET_BASE
  ${CMAKE_CURRENT_SOURCE_DIR})
string(REGEX REPLACE
  "/zephyr/tests/" "/src/"
  BACNET_SRC_PATH
  ${CMAKE_CURRENT_SOURCE_DIR})
string(REGEX REPLACE
  "/zephyr/tests/" "/test/"
  BACNET_TEST_PATH
  ${CMAKE_CURRENT_SOURCE_DIR})
get_filename_component(BACNET_NAME ${BACNET_BASE} NAME)

add_definitions(-DMAX_APDU=1476)
get_filename_component(BACNET_DATALINK_SRC ${BACNET_SRC_PATH} PATH)

if(BOARD STREQUAL unit_testing)
  file(RELATIVE_PATH BACNET_INCLUDE $ENV{ZEPHYR_BASE} ${BACNET_BASE}/src)
  list(APPEND INCLUDE ${BACNET_INCLUDE})
  list(APPEND SOURCES
    ${BACNET_SRC_PATH}.c
    ${BACNET_TEST_PATH}/src/main.c
    ${BACNET_DATALINK_SRC}/cobs.c
    )

  add_definitions(-DBACDL_MSTP=1)

  include($ENV{ZEPHYR_BASE}/subsys/testsuite/unittest.cmake)
  project(${BACNET_NAME})
else()
  include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
  project(${BACNET_NAME})

  target_include_directories(app PRIVATE ${BACNET_BASE}/src)
  target_sources(app PRIVATE
    ${BACNET_TEST_PATH}/src/main.c
    ${BACNET_DATALINK_SRC}/cobs.c
    )
endif()
