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

# Update include path for this module
list(APPEND BACNET_INCLUDE ${BACNET_BASE}/src)

if(BOARD STREQUAL unit_testing)
  file(RELATIVE_PATH BACNET_INCLUDE $ENV{ZEPHYR_BASE} ${BACNET_BASE}/src)
  list(APPEND INCLUDE ${BACNET_INCLUDE})
  list(APPEND SOURCES
    ${BACNET_SRC_PATH}.c
    ${BACNET_TEST_PATH}/src/main.c
    )

  get_filename_component(BACNET_DATALINK_SRC ${BACNET_SRC_PATH} PATH)
  get_filename_component(BACNET_SRC ${BACNET_DATALINK_SRC} PATH)
  list(APPEND SOURCES
    ${BACNET_SRC}/bacdcode.c
    ${BACNET_SRC}/bacstr.c
    ${BACNET_SRC}/bacint.c
    ${BACNET_SRC}/bacreal.c
    ${BACNET_SRC}/datetime.c
    ${BACNET_SRC}/timestamp.c
    ${BACNET_SRC}/basic/sys/days.c
    ${BACNET_SRC}/bacdevobjpropref.c
    )

  # BIP Options
  add_definitions(-DBACDL_BIP=1 -DBACDL_BIP_PORT=47808
    -DBACDL_BIP_ADDRESS_INDEX=0)
  # pthreads
  add_definitions(-DPOSIX_API=1 -DPTHREAD_IPC=1 -DPOSIX_MQUEUE=1)
  # networking
  add_definitions(-DNETWORKING=1 -DNET_IPV4=1 -DNET_ARP=1 -DNET_TCP=1
    -DNET_UDP=1 -DNET_DHCPV4=1)
  add_definitions(-DNET_MGMT=1 -DNET_MGMT_EVENT=1)

  include($ENV{ZEPHYR_BASE}/subsys/testsuite/unittest.cmake)
  project(${BACNET_NAME})
else()
  include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
  project(${BACNET_NAME})

  target_include_directories(app PRIVATE ${BACNET_INCLUDE})
  target_sources(app PRIVATE
    ${BACNET_TEST_PATH}/src/main.c
    )
endif()
