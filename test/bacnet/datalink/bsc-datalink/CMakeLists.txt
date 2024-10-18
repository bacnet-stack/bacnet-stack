# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
get_filename_component(basename ${CMAKE_CURRENT_SOURCE_DIR} NAME)

project(test_${basename}
  VERSION 1.0.0
  LANGUAGES C)

find_package(Threads)
find_package(libwebsockets CONFIG REQUIRED)
find_package(PkgConfig)
pkg_check_modules(LIB_WEBSOCKETS REQUIRED libwebsockets)
find_package(OpenSSL)

string(REGEX REPLACE
    "/test/bacnet/[a-zA-Z_/-]*$"
    "/src"
    SRC_DIR
    ${CMAKE_CURRENT_SOURCE_DIR})
string(REGEX REPLACE
    "/test/bacnet/[a-zA-Z_/-]*$"
    "/ports"
    PORTS_DIR
    ${CMAKE_CURRENT_SOURCE_DIR})
string(REGEX REPLACE
    "/test/bacnet/[a-zA-Z_/-]*$"
    "/test"
    TST_DIR
    ${CMAKE_CURRENT_SOURCE_DIR})
set(ZTST_DIR "${TST_DIR}/ztest/src")

add_compile_definitions(
  BIG_ENDIAN=0
  CONFIG_ZTEST=1
  BACDL_BSC
  MAX_BACFILES=4
  BSC_CONF_WSURL_MAX_LEN=128
  BSC_CONF_WEBSOCKET_ERR_DESC_STR_MAX_LEN=128
  BSC_CONF_HUB_FUNCTION_CONNECTIONS_NUM=3
  BSC_CONF_NODE_SWITCH_CONNECTIONS_NUM=3
  BSC_CONF_WEBSOCKET_SERVERS_NUM=6
  BSC_CONF_HUB_CONNECTORS_NUM=4
  BSC_CONF_HUB_FUNCTIONS_NUM=4
  BSC_CONF_NODES_NUM=4
  BSC_CONF_NODE_SWITCHES_NUM=4
  BSC_CONF_FAILED_CONNECTION_STATUS_MAX_NUM=1
  BSC_CONF_HUB_FUNCTION_CONNECTION_STATUS_MAX_NUM=2
  BSC_CONF_NODE_SWITCH_CONNECTION_STATUS_MAX_NUM=2
  MAX_TSM_TRANSACTIONS=0
  BACDL_BSC
  )

include_directories(
  ${SRC_DIR}
  ${TST_DIR}/ztest/include
  )

if(ZEPHYR_BASE)
  message(FATAL_ERROR "ZEPHYR_BASE env variable defined. Use zephyr/CMakeLists.txt for Zephyr build")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  message(STATUS "BACNet/SC datalink test: building for linux")
  set(BACNET_PORT_DIRECTORY_PATH ${CMAKE_CURRENT_LIST_DIR}/ports/linux)
  add_compile_definitions(BACNET_PORT=linux)

  add_executable(${PROJECT_NAME}
    ${PORTS_DIR}/linux/websocket-cli.c
    ${PORTS_DIR}/linux/websocket-srv.c
    ${PORTS_DIR}/linux/websocket-mutex.c
    ${PORTS_DIR}/linux/bsc-event.c
    ${PORTS_DIR}/linux/mstimer-init.c
    ${PORTS_DIR}/linux/datetime-init.c
    ${SRC_DIR}/bacnet/datalink/bsc/bsc-util.c
    ${SRC_DIR}/bacnet/datalink/bsc/bvlc-sc.c
    ${SRC_DIR}/bacnet/datalink/bsc/bsc-socket.c
    ${SRC_DIR}/bacnet/datalink/bsc/bsc-hub-connector.c
    ${SRC_DIR}/bacnet/datalink/bsc/bsc-hub-function.c
    ${SRC_DIR}/bacnet/datalink/bsc/bsc-node-switch.c
    ${SRC_DIR}/bacnet/datalink/bsc/bsc-node.c
    ${SRC_DIR}/bacnet/datalink/bsc/bsc-datalink.c
    ${SRC_DIR}/bacnet/basic/service/h_apdu.c
    ${SRC_DIR}/bacnet/basic/object/bacfile.c
    ${SRC_DIR}/bacnet/basic/object/netport.c
    ${SRC_DIR}/bacnet/basic/object/sc_netport.c
    ${SRC_DIR}/bacnet/basic/sys/days.c
    ${SRC_DIR}/bacnet/basic/sys/debug.c
    ${SRC_DIR}/bacnet/basic/sys/fifo.c
    ${SRC_DIR}/bacnet/basic/sys/keylist.c
    ${SRC_DIR}/bacnet/basic/sys/mstimer.c
    ${SRC_DIR}/bacnet/access_rule.c
    ${SRC_DIR}/bacnet/arf.c
    ${SRC_DIR}/bacnet/bacaction.c
    ${SRC_DIR}/bacnet/bacaddr.c
    ${SRC_DIR}/bacnet/bacapp.c
    ${SRC_DIR}/bacnet/bacaddr.c
    ${SRC_DIR}/bacnet/bacdcode.c
    ${SRC_DIR}/bacnet/bacdest.c
    ${SRC_DIR}/bacnet/bacdevobjpropref.c
    ${SRC_DIR}/bacnet/bacerror.c
    ${SRC_DIR}/bacnet/bacint.c
    ${SRC_DIR}/bacnet/bacreal.c
    ${SRC_DIR}/bacnet/bacstr.c
    ${SRC_DIR}/bacnet/bactext.c
    ${SRC_DIR}/bacnet/calendar_entry.c
    ${SRC_DIR}/bacnet/datetime.c
    ${SRC_DIR}/bacnet/bactimevalue.c
    ${SRC_DIR}/bacnet/weeklyschedule.c
    ${SRC_DIR}/bacnet/dailyschedule.c
    ${SRC_DIR}/bacnet/dcc.c
    ${SRC_DIR}/bacnet/indtext.c
    ${SRC_DIR}/bacnet/hostnport.c
    ${SRC_DIR}/bacnet/lighting.c
    ${SRC_DIR}/bacnet/npdu.c
    ${SRC_DIR}/bacnet/proplist.c
    ${SRC_DIR}/bacnet/special_event.c
    ${SRC_DIR}/bacnet/timestamp.c
    ${SRC_DIR}/bacnet/wp.c
    ${SRC_DIR}/bacnet/basic/sys/bigend.c
    # Test and test library files
    ./src/main.c
    ${TST_DIR}/bacnet/basic/object/test/device_mock.c
    ${TST_DIR}/bacnet/basic/object/test/property_test.c
    ${ZTST_DIR}/ztest_mock.c
    ${ZTST_DIR}/ztest.c
    )
  target_link_libraries(${PROJECT_NAME}
                        ${LIB_WEBSOCKETS_LIBRARIES}
  )
elseif(WIN32)
  message(STATUS "BACNet/SC datalink test: building for win32")
  set(BACNET_PORT_DIRECTORY_PATH ${CMAKE_CURRENT_LIST_DIR}/ports/win32)
  add_compile_definitions(BACNET_PORT=win32)
  target_link_libraries(${PROJECT_NAME} PUBLIC wsock32)

  target_link_libraries(${PROJECT_NAME} PRIVATE
    winmm
    ws2_32>
    iphlpapi>)

  add_executable(${PROJECT_NAME}
    ${PORTS_DIR}/win32/websocket-cli.c
    ${PORTS_DIR}/win32/websocket-srv.c
    ${PORTS_DIR}/win32/bsc-event.c
    ${PORTS_DIR}/win32/mstimer-init.c
    ${PORTS_DIR}/win32/datetime-init.c
    ${SRC_DIR}/bacnet/datalink/bsc/bsc-util.c
    ${SRC_DIR}/bacnet/datalink/bsc/bvlc-sc.c
    ${SRC_DIR}/bacnet/datalink/bsc/bsc-socket.c
    ${SRC_DIR}/bacnet/datalink/bsc/bsc-hub-connector.c
    ${SRC_DIR}/bacnet/datalink/bsc/bsc-hub-function.c
    ${SRC_DIR}/bacnet/datalink/bsc/bsc-node-switch.c
    ${SRC_DIR}/bacnet/datalink/bsc/bsc-node.c
    ${SRC_DIR}/bacnet/datalink/bsc/bsc-datalink.c
    ${SRC_DIR}/bacnet/basic/service/h_apdu.c
    ${SRC_DIR}/bacnet/basic/object/bacfile.c
    ${SRC_DIR}/bacnet/basic/object/netport.c
    ${SRC_DIR}/bacnet/basic/object/sc_netport.c
    ${SRC_DIR}/bacnet/basic/sys/days.c
    ${SRC_DIR}/bacnet/basic/sys/debug.c
    ${SRC_DIR}/bacnet/basic/sys/fifo.c
    ${SRC_DIR}/bacnet/basic/sys/keylist.c
    ${SRC_DIR}/bacnet/basic/sys/mstimer.c
    ${SRC_DIR}/bacnet/access_rule.c
    ${SRC_DIR}/bacnet/arf.c
    ${SRC_DIR}/bacnet/bacaction.c
    ${SRC_DIR}/bacnet/bacaddr.c
    ${SRC_DIR}/bacnet/bacapp.c
    ${SRC_DIR}/bacnet/bacaddr.c
    ${SRC_DIR}/bacnet/bacdcode.c
    ${SRC_DIR}/bacnet/bacdest.c
    ${SRC_DIR}/bacnet/bacdevobjpropref.c
    ${SRC_DIR}/bacnet/bacerror.c
    ${SRC_DIR}/bacnet/bacint.c
    ${SRC_DIR}/bacnet/bacreal.c
    ${SRC_DIR}/bacnet/bacstr.c
    ${SRC_DIR}/bacnet/bactext.c
    ${SRC_DIR}/bacnet/calendar_entry.c
    ${SRC_DIR}/bacnet/datetime.c
    ${SRC_DIR}/bacnet/bactimevalue.c
    ${SRC_DIR}/bacnet/weeklyschedule.c
    ${SRC_DIR}/bacnet/dailyschedule.c
    ${SRC_DIR}/bacnet/dcc.c
    ${SRC_DIR}/bacnet/indtext.c
    ${SRC_DIR}/bacnet/hostnport.c
    ${SRC_DIR}/bacnet/lighting.c
    ${SRC_DIR}/bacnet/npdu.c
    ${SRC_DIR}/bacnet/proplist.c
    ${SRC_DIR}/bacnet/special_event.c
    ${SRC_DIR}/bacnet/timestamp.c
    ${SRC_DIR}/bacnet/wp.c
    ${SRC_DIR}/bacnet/basic/sys/bigend.c
    # Test and test library files
    ./src/main.c
    ${TST_DIR}/bacnet/basic/object/test/device_mock.c
    ${TST_DIR}/bacnet/basic/object/test/property_test.c
    ${ZTST_DIR}/ztest_mock.c
    ${ZTST_DIR}/ztest.c
    )

elseif(APPLE)
  message(STATUS "BACNet/SC datalink test: building for APPLE")
  set(BACNET_PORT_DIRECTORY_PATH ${CMAKE_CURRENT_LIST_DIR}/ports/bsd)
  add_compile_definitions(BACNET_PORT=bsd)
  add_executable(${PROJECT_NAME}
    ${PORTS_DIR}/bsd/websocket-cli.c
    ${PORTS_DIR}/bsd/websocket-srv.c
    ${PORTS_DIR}/bsd/websocket-mutex.c
    ${PORTS_DIR}/bsd/bsc-event.c
    ${PORTS_DIR}/bsd/mstimer-init.c
    ${PORTS_DIR}/bsd/datetime-init.c
    ${SRC_DIR}/bacnet/datalink/bsc/bsc-util.c
    ${SRC_DIR}/bacnet/datalink/bsc/bvlc-sc.c
    ${SRC_DIR}/bacnet/datalink/bsc/bsc-socket.c
    ${SRC_DIR}/bacnet/datalink/bsc/bsc-hub-connector.c
    ${SRC_DIR}/bacnet/datalink/bsc/bsc-hub-function.c
    ${SRC_DIR}/bacnet/datalink/bsc/bsc-node-switch.c
    ${SRC_DIR}/bacnet/datalink/bsc/bsc-node.c
    ${SRC_DIR}/bacnet/datalink/bsc/bsc-datalink.c
    ${SRC_DIR}/bacnet/basic/service/h_apdu.c
    ${SRC_DIR}/bacnet/basic/object/bacfile.c
    ${SRC_DIR}/bacnet/basic/object/netport.c
    ${SRC_DIR}/bacnet/basic/object/sc_netport.c
    ${SRC_DIR}/bacnet/basic/sys/days.c
    ${SRC_DIR}/bacnet/basic/sys/debug.c
    ${SRC_DIR}/bacnet/basic/sys/fifo.c
    ${SRC_DIR}/bacnet/basic/sys/keylist.c
    ${SRC_DIR}/bacnet/basic/sys/mstimer.c
    ${SRC_DIR}/bacnet/access_rule.c
    ${SRC_DIR}/bacnet/arf.c
    ${SRC_DIR}/bacnet/bacaction.c
    ${SRC_DIR}/bacnet/bacaddr.c
    ${SRC_DIR}/bacnet/bacapp.c
    ${SRC_DIR}/bacnet/bacaddr.c
    ${SRC_DIR}/bacnet/bacdcode.c
    ${SRC_DIR}/bacnet/bacdest.c
    ${SRC_DIR}/bacnet/bacdevobjpropref.c
    ${SRC_DIR}/bacnet/bacerror.c
    ${SRC_DIR}/bacnet/bacint.c
    ${SRC_DIR}/bacnet/bacreal.c
    ${SRC_DIR}/bacnet/bacstr.c
    ${SRC_DIR}/bacnet/bactext.c
    ${SRC_DIR}/bacnet/calendar_entry.c
    ${SRC_DIR}/bacnet/datetime.c
    ${SRC_DIR}/bacnet/bactimevalue.c
    ${SRC_DIR}/bacnet/weeklyschedule.c
    ${SRC_DIR}/bacnet/dailyschedule.c
    ${SRC_DIR}/bacnet/dcc.c
    ${SRC_DIR}/bacnet/indtext.c
    ${SRC_DIR}/bacnet/hostnport.c
    ${SRC_DIR}/bacnet/lighting.c
    ${SRC_DIR}/bacnet/npdu.c
    ${SRC_DIR}/bacnet/proplist.c
    ${SRC_DIR}/bacnet/special_event.c
    ${SRC_DIR}/bacnet/timestamp.c
    ${SRC_DIR}/bacnet/wp.c
    ${SRC_DIR}/bacnet/basic/sys/bigend.c
    # Test and test library files
    ./src/main.c
    ${TST_DIR}/bacnet/basic/object/test/device_mock.c
    ${TST_DIR}/bacnet/basic/object/test/property_test.c
    ${ZTST_DIR}/ztest_mock.c
    ${ZTST_DIR}/ztest.c
    )
  target_link_libraries(${PROJECT_NAME}
                        ${LIB_WEBSOCKETS_LIBRARIES}
  )
endif()
