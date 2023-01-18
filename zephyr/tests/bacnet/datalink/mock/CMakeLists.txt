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

get_filename_component(BACNET_DATALINK_SRC ${BACNET_SRC_PATH} PATH)
get_filename_component(BACNET_SRC ${BACNET_DATALINK_SRC} PATH)

if(BOARD STREQUAL unit_testing)
  file(RELATIVE_PATH BACNET_INCLUDE $ENV{ZEPHYR_BASE} ${BACNET_BASE}/src)
  list(APPEND INCLUDE ${BACNET_INCLUDE})

  file(GLOB SRC_TEST ${BACNET_TEST_PATH}/src/*.c)
  list(APPEND SOURCES ${SRC_TEST} )

  list(APPEND SOURCES
    ${BACNET_DATALINK_SRC}/datalink.c
    )

  add_definitions(-DBACDL_ALL=1)

  set(CONF_FILE "${CONF_FILE};prj.unit_testing.conf")
  find_package(Zephyr COMPONENTS unittest REQUIRED HINTS $ENV{ZEPHYR_BASE})
  project(${BACNET_NAME})
else()
  find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
  project(${BACNET_NAME})

  target_include_directories(app PRIVATE ${BACNET_INCLUDE})
  file(GLOB SRC_TEST ${BACNET_TEST_PATH}/src/*.c)

  add_definitions(-DBACDL_ALL=1 -DBACNET_ETHERNET_NO_REMAP_DEFINES=1)

  target_sources(app PRIVATE
    ${BACNET_DATALINK_SRC}/datalink.c
    ${SRC_TEST}
    )
endif()
