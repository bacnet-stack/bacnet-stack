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

add_compile_definitions(
	BACNET_SECURE_CONNECT_ROUTING_TABLE=1
	BSC_CONF_HUB_FUNCTIONS_NUM=1
	BSC_CONF_HUB_CONNECTORS_NUM=1
	BACNET_SC_DIRECT_ACCEPT_URI_MAX=6
	BSC_CONF_TX_PRE=0
	BACDL_BSC
	MAX_TSM_TRANSACTIONS=0
)

if(BOARD STREQUAL unit_testing)
  file(RELATIVE_PATH BACNET_INCLUDE $ENV{ZEPHYR_BASE} ${BACNET_BASE}/src)
  list(APPEND INCLUDE ${BACNET_INCLUDE})
  list(APPEND SOURCES
    ${BACNET_SRC_PATH}.c
    ${BACNET_TEST_PATH}/src/main.c
    ${BACNET_TEST_PATH}/../mock/device_mock.c
    )

  get_filename_component(BACNET_OBJECT_SRC ${BACNET_SRC_PATH} PATH)
  get_filename_component(BACNET_BASIC_SRC ${BACNET_OBJECT_SRC} PATH)
  get_filename_component(BACNET_SRC ${BACNET_BASIC_SRC} PATH)
  list(APPEND SOURCES
    ${BACNET_SRC}/bacaddr.c
    ${BACNET_SRC}/bacapp.c
    ${BACNET_SRC}/bacdcode.c
    ${BACNET_SRC}/bacdest.c
    ${BACNET_SRC}/bacstr.c
    ${BACNET_SRC}/bacint.c
    ${BACNET_SRC}/bacreal.c
    ${BACNET_SRC}/datetime.c
    ${BACNET_SRC}/timestamp.c
    ${BACNET_SRC}/basic/sys/days.c
    ${BACNET_SRC}/basic/sys/keylist.c
    ${BACNET_SRC}/bacdevobjpropref.c
    ${BACNET_SRC}/bactext.c
    ${BACNET_SRC}/indtext.c
    ${BACNET_SRC}/lighting.c
    ${BACNET_SRC}/wp.c
    ${BACNET_SRC}/proplist.c
    ${BACNET_SRC}/datalink/bvlc.c
    ${BACNET_SRC}/basic/object/bacfile.c
    ${BACNET_SRC}/sc_status.c
    ${BACNET_SRC}/basic/object/sc_netport.c
    ${BACNET_SRC}/hostnport.c
    ${BACNET_SRC}/dailyschedule.c
    ${BACNET_SRC}/weeklyschedule.c
    ${BACNET_SRC}/basic/sys/bigend.c
    ${BACNET_SRC}/bactimevalue.c
    ${BACNET_SRC}/datalink/bsc/bsc-util.c
    ${BACNET_SRC}/basic/sys/debug.c
    )

  set(CONF_FILE "${CONF_FILE};prj.unit_testing.conf")
  find_package(Zephyr COMPONENTS unittest REQUIRED HINTS $ENV{ZEPHYR_BASE})
  project(${BACNET_NAME})
else()
  find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
  project(${BACNET_NAME})

  target_include_directories(app PRIVATE ${BACNET_INCLUDE})
  target_sources(app PRIVATE
    ${BACNET_TEST_PATH}/src/main.c
    )
endif()
