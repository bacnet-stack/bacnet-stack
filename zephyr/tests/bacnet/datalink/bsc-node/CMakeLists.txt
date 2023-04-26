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

add_compile_definitions(
  BIG_ENDIAN=0
  #CONFIG_ZTEST=1
  BACDL_BSC
  BSC_CONF_HUB_FUNCTION_CONNECTIONS_NUM=4
  BSC_CONF_NODE_SWITCH_CONNECTIONS_NUM=4
  BSC_CONF_WSURL_MAX_LEN=127
  BSC_CONF_WEBSOCKET_SERVERS_NUM=4
  BSC_CONF_HUB_CONNECTORS_NUM=3
  BSC_CONF_HUB_FUNCTIONS_NUM=3
  BSC_CONF_NODES_NUM=3
  BSC_CONF_NODE_SWITCHES_NUM=3
  MAX_TSM_TRANSACTIONS=0
  ZEPHYR_TEST
  )

if(BOARD STREQUAL unit_testing)
  get_filename_component(BACNET_DATALINK_PATH ${BACNET_SRC_PATH} PATH)

  file(RELATIVE_PATH BACNET_INCLUDE $ENV{ZEPHYR_BASE} ${BACNET_BASE}/src)
  list(APPEND INCLUDE ${BACNET_INCLUDE})
  list(APPEND SOURCES
    ${BACNET_BASE}/ports/zephyr/websocket-cli.c
    ${BACNET_BASE}/ports/zephyr/websocket-srv.c
    ${BACNET_BASE}/ports/zephyr/websocket-mutex.c
    ${BACNET_BASE}/ports/zephyr/bsc-event.c
    ${BACNET_BASE}/ports/zephyr/mstimer-init.c
    ${BACNET_BASE}/ports/zephyr/datetime-init.c
    ${BACNET_DATALINK_PATH}/bsc/bsc-util.c
    ${BACNET_DATALINK_PATH}/bsc/bvlc-sc.c
    ${BACNET_DATALINK_PATH}/bsc/bsc-socket.c
    ${BACNET_DATALINK_PATH}/bsc/bsc-hub-connector.c
    ${BACNET_DATALINK_PATH}/bsc/bsc-hub-function.c
    ${BACNET_DATALINK_PATH}/bsc/bsc-node-switch.c
    ${BACNET_DATALINK_PATH}/bsc/bsc-node.c
    ${BACNET_DATALINK_PATH}/bsc/bsc-datalink.c
    ${BACNET_SRC_PATH}/basic/service/h_apdu.c
    ${BACNET_SRC_PATH}/basic/object/bacfile.c
    ${BACNET_SRC_PATH}/basic/object/netport.c
    ${BACNET_SRC_PATH}/basic/object/sc_netport.c
    ${BACNET_SRC_PATH}/basic/sys/days.c
    ${BACNET_SRC_PATH}/basic/sys/debug.c
    ${BACNET_SRC_PATH}/basic/sys/fifo.c
    ${BACNET_SRC_PATH}/basic/sys/keylist.c
    ${BACNET_SRC_PATH}/basic/sys/mstimer.c
    ${BACNET_SRC_PATH}/arf.c
    ${BACNET_SRC_PATH}/bacapp.c
    ${BACNET_SRC_PATH}/bacaddr.c
    ${BACNET_SRC_PATH}/bacdcode.c
    ${BACNET_SRC_PATH}/bacdevobjpropref.c
    ${BACNET_SRC_PATH}/bacerror.c
    ${BACNET_SRC_PATH}/bacint.c
    ${BACNET_SRC_PATH}/bacreal.c
    ${BACNET_SRC_PATH}/bacstr.c
    ${BACNET_SRC_PATH}/bactext.c
    ${BACNET_SRC_PATH}/datetime.c
    ${BACNET_SRC_PATH}/bactimevalue.c
    ${BACNET_SRC_PATH}/weeklyschedule.c
    ${BACNET_SRC_PATH}/dailyschedule.c
    ${BACNET_SRC_PATH}/dcc.c
    ${BACNET_SRC_PATH}/indtext.c
    ${BACNET_SRC_PATH}/hostnport.c
    ${BACNET_SRC_PATH}/lighting.c
    ${BACNET_SRC_PATH}/npdu.c
    ${BACNET_SRC_PATH}/proplist.c
    ${BACNET_SRC_PATH}/timestamp.c
    ${BACNET_SRC_PATH}/wp.c
    # Test and test library files
    ${BACNET_TEST_PATH}/src/main.c
    #${CMAKE_CURRENT_SOURCE_DIR}/src/utils.c
    )

  include($ENV{ZEPHYR_BASE}/subsys/testsuite/unittest.cmake)
  project(${BACNET_NAME})
else()
  include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
  project(${BACNET_NAME})

  target_include_directories(app PRIVATE ${BACNET_BASE}/src)
  target_sources(app PRIVATE
    ${BACNET_TEST_PATH}/src/main.c
    #${CMAKE_CURRENT_SOURCE_DIR}/src/utils.c
    )
endif()
