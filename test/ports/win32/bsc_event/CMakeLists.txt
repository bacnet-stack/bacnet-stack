# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
get_filename_component(basename ${CMAKE_CURRENT_SOURCE_DIR} NAME)

project(test_${basename}
	VERSION 1.0.0
	LANGUAGES C)

find_package(Threads)
find_package(PkgConfig)

string(REGEX REPLACE
    "/test/ports/[a-zA-Z_/-]*$"
    "/src"
    SRC_DIR
    ${CMAKE_CURRENT_SOURCE_DIR})
string(REGEX REPLACE
    "/test/ports/[a-zA-Z_/-]*$"
    "/ports"
    PORTS_DIR
    ${CMAKE_CURRENT_SOURCE_DIR})
string(REGEX REPLACE
    "/test/ports/[a-zA-Z_/-]*$"
    "/test"
    TST_DIR
    ${CMAKE_CURRENT_SOURCE_DIR})
set(ZTST_DIR "${TST_DIR}/ztest/src")

add_compile_definitions(
	BIG_ENDIAN=0
	CONFIG_ZTEST=1
	)

include_directories(
	${SRC_DIR}
	${TST_DIR}/ztest/include
	)

message(STATUS "bsc_event test: building for win32")
set(BACNET_PORT_DIRECTORY_PATH ${PORTS_DIR})

target_link_libraries(${PROJECT_NAME} PUBLIC wsock32)

target_link_libraries(${PROJECT_NAME} PRIVATE
  winmm
  ws2_32>
  iphlpapi>)

add_executable(${PROJECT_NAME}
  ${PORTS_DIR}/win32/bsc-event.c
  # Test and test library files
  ./src/main.c
  ${ZTST_DIR}/ztest_mock.c
  ${ZTST_DIR}/ztest.c
  )
