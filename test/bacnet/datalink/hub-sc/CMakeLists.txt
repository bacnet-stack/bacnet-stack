# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
get_filename_component(basename ${CMAKE_CURRENT_SOURCE_DIR} NAME)

project(test_${basename}
  VERSION 1.0.0
  LANGUAGES C)

string(REGEX REPLACE
    "/test/bacnet/[a-z0-9A-Z_/-]*$"
    "/src"
    SRC_DIR
    ${CMAKE_CURRENT_SOURCE_DIR})
string(REGEX REPLACE
    "/test/bacnet/[a-z0-9A-Z_/-]*$"
    "/ports"
    PORTS_DIR
    ${CMAKE_CURRENT_SOURCE_DIR})
string(REGEX REPLACE
    "/test/bacnet/[a-z0-9A-Z_/-]*$"
    "/test"
    TST_DIR
    ${CMAKE_CURRENT_SOURCE_DIR})
set(ZTST_DIR "${TST_DIR}/ztest/src")

add_compile_definitions(
  BIG_ENDIAN=0
  CONFIG_ZTEST=1
  BACDL_BSC=1
  BACNET_STACK_DEPRECATED_DISABLE=1
  BSC_CONF_HUB_FUNCTION_CONNECTIONS_NUM=4
  BSC_CONF_NODE_SWITCH_CONNECTIONS_NUM=4
  BSC_CONF_WSURL_MAX_LEN=128
  BSC_CONF_WEBSOCKET_ERR_DESC_STR_MAX_LEN=128
  BSC_CONF_WEBSOCKET_SERVERS_NUM=4
  BSC_CONF_HUB_CONNECTORS_NUM=2
  BSC_CONF_HUB_FUNCTIONS_NUM=2
  MAX_TSM_TRANSACTIONS=0
  )

include_directories(
  ${SRC_DIR}
  ${TST_DIR}/ztest/include
  )

if(ZEPHYR_BASE)
  message(FATAL_ERROR "ZEPHYR_BASE env variable defined. Use zephyr/CMakeLists.txt for Zephyr build")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  message(STATUS "BACnet/SC hub function test: building for linux")
  set(BACNET_PORT_DIRECTORY_PATH ${CMAKE_CURRENT_LIST_DIR}/ports/linux)
  add_compile_definitions(BACNET_PORT=linux)
  find_package(libwebsockets CONFIG REQUIRED)
  include_directories(${LIBWEBSOCKETS_INCLUDE_DIRS})

  add_executable(${PROJECT_NAME}
    ${PORTS_DIR}/linux/websocket-cli.c
    ${PORTS_DIR}/linux/websocket-srv.c
    ${PORTS_DIR}/linux/websocket-global.c
    ${PORTS_DIR}/linux/bsc-event.c
    ${PORTS_DIR}/linux/mstimer-init.c
    ${PORTS_DIR}/linux/datetime-init.c
  )
  target_link_libraries(${PROJECT_NAME}
    ${LIBWEBSOCKETS_LIBRARIES}
    )
  target_compile_options(${PROJECT_NAME} PRIVATE
    -Wno-language-extension-token
    )
elseif(WIN32)
  message(STATUS "BACnet/SC hub function test: building for win32")
  set(BACNET_PORT_DIRECTORY_PATH ${CMAKE_CURRENT_LIST_DIR}/ports/win32)
  add_compile_definitions(BACNET_PORT=win32)
  add_compile_definitions(BACNET_STACK_STATIC_DEFINE)
  add_compile_definitions(BACNET_USE_DOUBLE=0)
  find_package(libwebsockets CONFIG REQUIRED)
  include_directories(${LIBWEBSOCKETS_INCLUDE_DIRS})
  find_package(OpenSSL)

  add_executable(${PROJECT_NAME}
    ${PORTS_DIR}/win32/websocket-cli.c
    ${PORTS_DIR}/win32/websocket-srv.c
    ${PORTS_DIR}/win32/websocket-global.c
    ${PORTS_DIR}/win32/bsc-event.c
    ${PORTS_DIR}/win32/mstimer-init.c
    ${PORTS_DIR}/win32/datetime-init.c
  )

  # basically if you use vcpkg you should just add ${LIBWEBSOCKETS_LIBRARIES}
  # into target_link_libraries() but for some reason it does not work as expected
  # so that's why libs have to be hardcoded as workaround
  target_link_libraries(${PROJECT_NAME}
                        C:\\vcpkg\\installed\\x64-windows-custom\\lib\\websockets.lib
                        C:\\vcpkg\\installed\\x64-windows-custom\\lib\\libssl.lib
                        C:\\vcpkg\\installed\\x64-windows-custom\\lib\\libcrypto.lib
                        C:\\vcpkg\\installed\\x64-windows-custom\\lib\\pthreadVC3.lib
                        ws2_32.lib
                        userenv.lib
                        psapi.lib
                        iphlpapi.lib
                        crypt32.lib
                        C:\\vcpkg\\installed\\x64-windows-custom\\lib\\zlib.lib
                        C:\\vcpkg\\installed\\x64-windows-custom\\lib\\uv.lib
                        kernel32.lib
                        user32.lib
                        gdi32.lib
                        winspool.lib
                        shell32.lib
                        ole32.lib
                        oleaut32.lib
                        uuid.lib
                        comdlg32.lib
                        advapi32.lib
  )

elseif(APPLE)
  message(STATUS "BACnet/SC hub function test: building for APPLE")
  set(BACNET_PORT_DIRECTORY_PATH ${CMAKE_CURRENT_LIST_DIR}/ports/bsd)
  execute_process (
         COMMAND bash -c "brew --prefix openssl"
         OUTPUT_VARIABLE OPEN_SSL_DIR OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  include_directories(${OPEN_SSL_DIR}/include)
  add_compile_definitions(BACNET_PORT=bsd)
  find_package(libwebsockets CONFIG REQUIRED)
  include_directories(${LIBWEBSOCKETS_INCLUDE_DIRS})

  add_executable(${PROJECT_NAME}
    ${PORTS_DIR}/bsd/websocket-cli.c
    ${PORTS_DIR}/bsd/websocket-srv.c
    ${PORTS_DIR}/bsd/websocket-global.c
    ${PORTS_DIR}/bsd/bsc-event.c
    ${PORTS_DIR}/bsd/mstimer-init.c
    ${PORTS_DIR}/bsd/datetime-init.c
  )
  target_link_libraries(${PROJECT_NAME}
    ${LIBWEBSOCKETS_LIBRARIES}
    )
  target_compile_options(${PROJECT_NAME} PRIVATE
    -Wno-language-extension-token
    )
endif()

target_sources(${PROJECT_NAME} PRIVATE
  ${SRC_DIR}/bacnet/datalink/bsc/bsc-util.c
  ${SRC_DIR}/bacnet/datalink/bsc/bvlc-sc.c
  ${SRC_DIR}/bacnet/datalink/bsc/bsc-socket.c
  ${SRC_DIR}/bacnet/datalink/bsc/bsc-hub-connector.c
  ${SRC_DIR}/bacnet/datalink/bsc/bsc-hub-function.c
  ${SRC_DIR}/bacnet/datalink/bsc/bsc-node-switch.c
  ${SRC_DIR}/bacnet/datalink/bsc/bsc-node.c
  ${SRC_DIR}/bacnet/datalink/bsc/bsc-datalink.c
  ${SRC_DIR}/bacnet/datalink/bvlc.c
  ${SRC_DIR}/bacnet/datalink/bvlc6.c
  ${SRC_DIR}/bacnet/basic/service/h_apdu.c
  ${SRC_DIR}/bacnet/basic/object/bacfile.c
  ${SRC_DIR}/bacnet/basic/object/netport.c
  ${SRC_DIR}/bacnet/basic/object/sc_netport.c
  ${SRC_DIR}/bacnet/basic/sys/bigend.c
  ${SRC_DIR}/bacnet/basic/sys/days.c
  ${SRC_DIR}/bacnet/basic/sys/debug.c
  ${SRC_DIR}/bacnet/basic/sys/fifo.c
  ${SRC_DIR}/bacnet/basic/sys/keylist.c
  ${SRC_DIR}/bacnet/basic/sys/mstimer.c
  ${SRC_DIR}/bacnet/access_rule.c
  ${SRC_DIR}/bacnet/arf.c
  ${SRC_DIR}/bacnet/bacaction.c
  ${SRC_DIR}/bacnet/bacapp.c
  ${SRC_DIR}/bacnet/bacaddr.c
  ${SRC_DIR}/bacnet/bacdcode.c
  ${SRC_DIR}/bacnet/bacdest.c
  ${SRC_DIR}/bacnet/bacdevobjpropref.c
  ${SRC_DIR}/bacnet/bacerror.c
  ${SRC_DIR}/bacnet/bacint.c
  ${SRC_DIR}/bacnet/baclog.c
  ${SRC_DIR}/bacnet/bacreal.c
  ${SRC_DIR}/bacnet/bacstr.c
  ${SRC_DIR}/bacnet/bactext.c
  ${SRC_DIR}/bacnet/bactimevalue.c
  ${SRC_DIR}/bacnet/calendar_entry.c
  ${SRC_DIR}/bacnet/dailyschedule.c
  ${SRC_DIR}/bacnet/datetime.c
  ${SRC_DIR}/bacnet/dcc.c
  ${SRC_DIR}/bacnet/hostnport.c
  ${SRC_DIR}/bacnet/indtext.c
  ${SRC_DIR}/bacnet/lighting.c
  ${SRC_DIR}/bacnet/npdu.c
  ${SRC_DIR}/bacnet/proplist.c
  ${SRC_DIR}/bacnet/special_event.c
  ${SRC_DIR}/bacnet/channel_value.c
  ${SRC_DIR}/bacnet/secure_connect.c
  ${SRC_DIR}/bacnet/shed_level.c
  ${SRC_DIR}/bacnet/timer_value.c
  ${SRC_DIR}/bacnet/timestamp.c
  ${SRC_DIR}/bacnet/weeklyschedule.c
  ${SRC_DIR}/bacnet/wp.c
  # Test and test library files
  ./src/main.c
  ${TST_DIR}/bacnet/basic/object/test/device_mock.c
  ${ZTST_DIR}/ztest_mock.c
  ${ZTST_DIR}/ztest.c
)
