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

  get_filename_component(BACNET_SRC ${BACNET_SRC_PATH} PATH)
  list(APPEND SOURCES
    ${BACNET_SRC}/bacaddr.c
    ${BACNET_SRC}/bacapp.c
    ${BACNET_SRC}/bacstr.c
    ${BACNET_SRC}/bacdcode.c
    ${BACNET_SRC}/bacdest.c
    ${BACNET_SRC}/bacstr.c
    ${BACNET_SRC}/bacint.c
    ${BACNET_SRC}/bacreal.c
    ${BACNET_SRC}/datetime.c
    ${BACNET_SRC}/basic/sys/days.c
    ${BACNET_SRC}/bacdevobjpropref.c
    ${BACNET_SRC}/authentication_factor.c
    ${BACNET_SRC}/bacpropstates.c
    # Dependencies of bacapp.c
    ${BACNET_SRC}/bacaction.c
    ${BACNET_SRC}/bactext.c
    ${BACNET_SRC}/indtext.c
    ${BACNET_SRC}/weeklyschedule.c
    ${BACNET_SRC}/calendar_entry.c
    ${BACNET_SRC}/special_event.c
    ${BACNET_SRC}/bactimevalue.c
    ${BACNET_SRC}/dailyschedule.c
    ${BACNET_SRC}/lighting.c
    ${BACNET_SRC}/timestamp.c
    ${BACNET_SRC}/hostnport.c
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

add_compile_definitions(CONFIG_ZTEST_NEW_API)  # For exposing deprecated Kconfig
