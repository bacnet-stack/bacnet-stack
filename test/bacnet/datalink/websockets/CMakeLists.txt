# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
get_filename_component(basename ${CMAKE_CURRENT_SOURCE_DIR} NAME)

project(test_${basename}
    VERSION 1.0.0
    LANGUAGES C)

include(CMakePrintHelpers)

find_package(Threads)
find_package(libwebsockets CONFIG REQUIRED)
include_directories(${LIBWEBSOCKETS_INCLUDE_DIRS})

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
    ${LIBWEBSOCKETS_INCLUDE_DIRS}
    )

if(ZEPHYR_BASE)
  message(FATAL_ERROR "ZEPHYR_BASE env variable defined. Use zephyr/CMakeLists.txt for Zephyr build")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  message(STATUS "Websockets test: building for linux")
  set(BACNET_PORT_DIRECTORY_PATH ${CMAKE_CURRENT_LIST_DIR}/ports/linux)
  add_compile_definitions(BACNET_PORT=linux)
  find_package(OpenSSL)
  find_package(PkgConfig)
  pkg_check_modules(LIB_WEBSOCKETS REQUIRED libwebsockets)
  add_executable(${PROJECT_NAME}
    ${PORTS_DIR}/linux/websocket-cli.c
    ${PORTS_DIR}/linux/websocket-srv.c
    ${PORTS_DIR}/linux/websocket-global.c
    ${PORTS_DIR}/linux/bsc-event.c
    ${SRC_DIR}/bacnet/basic/sys/debug.c
    # Test and test library files
    ./src/main.c
    ${ZTST_DIR}/ztest_mock.c
    ${ZTST_DIR}/ztest.c
    )
    target_link_libraries(${PROJECT_NAME}
                          ${LIBWEBSOCKETS_LIBRARIES}
  )
elseif(WIN32)
  message(STATUS "Websockets test: building for win32")
  set(BACNET_PORT_DIRECTORY_PATH ${CMAKE_CURRENT_LIST_DIR}/ports/win32)
  add_compile_definitions(BACNET_PORT=win32)
  find_package(OpenSSL)
  add_executable(${PROJECT_NAME}
    ${PORTS_DIR}/win32/websocket-cli.c
    ${PORTS_DIR}/win32/websocket-srv.c
    ${PORTS_DIR}/win32/websocket-global.c
    ${PORTS_DIR}/win32/bsc-event.c
    # Test and test library files
    ./src/main.c
    ${ZTST_DIR}/ztest_mock.c
    ${ZTST_DIR}/ztest.c
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
  message(STATUS "Websockets test: building for APPLE")
  set(BACNET_PORT_DIRECTORY_PATH ${CMAKE_CURRENT_LIST_DIR}/ports/bsd)
  add_compile_definitions(BACNET_PORT=bsd)
  execute_process (
         COMMAND bash -c "brew --prefix openssl"
         OUTPUT_VARIABLE OPEN_SSL_DIR OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  include_directories(${OPEN_SSL_DIR}/include)
  add_executable(${PROJECT_NAME}
    ${PORTS_DIR}/bsd/websocket-cli.c
    ${PORTS_DIR}/bsd/websocket-srv.c
    ${PORTS_DIR}/bsd/websocket-global.c
    ${PORTS_DIR}/bsd/bsc-event.c
    ${SRC_DIR}/bacnet/basic/sys/debug.c
    # Test and test library files
    ./src/main.c
    ${ZTST_DIR}/ztest_mock.c
    ${ZTST_DIR}/ztest.c
    )
  target_link_libraries(${PROJECT_NAME}
                        ${LIBWEBSOCKETS_LIBRARIES}
  )
endif()
