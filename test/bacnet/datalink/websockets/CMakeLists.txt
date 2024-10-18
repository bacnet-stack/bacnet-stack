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
    BSC_CONF_CLIENT_CONNECTIONS_NUM=5
    BSC_CONF_SERVER_HUB_CONNECTIONS_MAX_NUM=4
    BSC_CONF_SERVER_DIRECT_CONNECTIONS_MAX_NUM=4
  BSC_CONF_WSURL_MAX_LEN=128
  BSC_CONF_WEBSOCKET_ERR_DESC_STR_MAX_LEN=128
  BSC_CONF_WEBSOCKET_SERVERS_NUM=4
    )

include_directories(
    ${SRC_DIR}
    ${TST_DIR}/ztest/include
    )

if(ZEPHYR_BASE)
  message(FATAL_ERROR "ZEPHYR_BASE env variable defined. Use zephyr/CMakeLists.txt for Zephyr build")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  message(STATUS "Websockets test: building for linux")
  message(STATUS "LIB_WEBSOCKETS_LIBRARIES= ${LIB_WEBSOCKETS_LIBRARIES}")
  set(BACNET_PORT_DIRECTORY_PATH ${CMAKE_CURRENT_LIST_DIR}/ports/linux)
  add_compile_definitions(BACNET_PORT=linux)
  add_executable(${PROJECT_NAME}
    ${PORTS_DIR}/linux/websocket-cli.c
    ${PORTS_DIR}/linux/websocket-srv.c
    ${PORTS_DIR}/linux/websocket-mutex.c
    ${SRC_DIR}/bacnet/basic/sys/debug.c
    # Test and test library files
    ./src/main.c
    ${ZTST_DIR}/ztest_mock.c
    ${ZTST_DIR}/ztest.c
    )
  target_link_libraries(${PROJECT_NAME}
                        ${LIB_WEBSOCKETS_LIBRARIES}
  )
elseif(WIN32)
  message(STATUS "Websockets test: building for win32")
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
    ${PORTS_DIR}/win32/websocket-mutex.c
    # Test and test library files
    ./src/main.c
    ${ZTST_DIR}/ztest_mock.c
    ${ZTST_DIR}/ztest.c
    )

elseif(APPLE)
  message(STATUS "Websockets test: building for APPLE")
  set(BACNET_PORT_DIRECTORY_PATH ${CMAKE_CURRENT_LIST_DIR}/ports/bsd)
  add_compile_definitions(BACNET_PORT=bsd)
  add_executable(${PROJECT_NAME}
    ${PORTS_DIR}/bsd/websocket-cli.c
    ${PORTS_DIR}/bsd/websocket-srv.c
    ${PORTS_DIR}/bsd/websocket-mutex.c
    ${SRC_DIR}/bacnet/basic/sys/debug.c
    # Test and test library files
    ./src/main.c
    ${ZTST_DIR}/ztest_mock.c
    ${ZTST_DIR}/ztest.c
    )
  target_link_libraries(${PROJECT_NAME}
                        ${LIB_WEBSOCKETS_LIBRARIES}
  )
endif()
