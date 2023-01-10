# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.10 FATAL_ERROR)

get_filename_component(basename ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project(test_${basename}
	VERSION 1.0.0
	LANGUAGES C)


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
	BACDL_BSC=1
	BACNET_SECURE_CONNECT_ROUTING_TABLE=1
	BSC_CONF_HUB_FUNCTIONS_NUM=1
	BSC_CONF_HUB_CONNECTORS_NUM=1
	BACNET_SC_DIRECT_ACCEPT_URI_MAX=6
	MAX_TSM_TRANSACTIONS=0
	)

include_directories(
	${SRC_DIR}
	${TST_DIR}/ztest/include
	)

set(SOURCES
    # File(s) under test
	${SRC_DIR}/bacnet/basic/object/bacfile.c
	${SRC_DIR}/bacnet/basic/object/netport.c
	${SRC_DIR}/bacnet/basic/object/sc_netport.c
    # Support files and stubs (pathname alphabetical)
	${SRC_DIR}/bacnet/arf.c
	${SRC_DIR}/bacnet/bacapp.c
	${SRC_DIR}/bacnet/bacdcode.c
	${SRC_DIR}/bacnet/bacdevobjpropref.c
	${SRC_DIR}/bacnet/bacerror.c
	${SRC_DIR}/bacnet/bacint.c
	${SRC_DIR}/bacnet/bacreal.c
	${SRC_DIR}/bacnet/bacstr.c
	${SRC_DIR}/bacnet/bactext.c
	${SRC_DIR}/bacnet/basic/sys/bigend.c
	${SRC_DIR}/bacnet/datalink/bvlc.c
	${SRC_DIR}/bacnet/datetime.c
	${SRC_DIR}/bacnet/basic/sys/days.c
	${SRC_DIR}/bacnet/basic/sys/keylist.c
	${SRC_DIR}/bacnet/indtext.c
	${SRC_DIR}/bacnet/hostnport.c
	${SRC_DIR}/bacnet/lighting.c
	${SRC_DIR}/bacnet/proplist.c
	${SRC_DIR}/bacnet/timestamp.c
	${SRC_DIR}/bacnet/wp.c
	${SRC_DIR}/bacnet/weeklyschedule.c
	${SRC_DIR}/bacnet/bactimevalue.c
	${SRC_DIR}/bacnet/dailyschedule.c
    # Test and test library files
	${TST_DIR}/bacnet/basic/object/mock/device_mock.c
	./src/main.c
	${ZTST_DIR}/ztest_mock.c
	${ZTST_DIR}/ztest.c
	)

if(ZEPHYR_BASE)
  message(FATAL_ERROR "ZEPHYR_BASE env variable defined. Use zephyr/CMakeLists.txt for Zephyr build")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  add_executable(${PROJECT_NAME} ${PORTS_DIR}/linux/bsc-mutex.c ${SOURCES})
elseif(WIN32)
  add_executable(${PROJECT_NAME} ${PORTS_DIR}/win32/bsc-mutex.c ${SOURCES})
elseif(APPLE)
  add_executable(${PROJECT_NAME} ${PORTS_DIR}/bsd/bsc-mutex.c ${SOURCES})
endif()
