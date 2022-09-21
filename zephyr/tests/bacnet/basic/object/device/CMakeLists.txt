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
    ${BACNET_TEST_PATH}/stubs.c
    )

  get_filename_component(BACNET_OBJECT_SRC ${BACNET_SRC_PATH} PATH)
  get_filename_component(BACNET_BASIC_SRC ${BACNET_OBJECT_SRC} PATH)
  get_filename_component(BACNET_SRC ${BACNET_BASIC_SRC} PATH)
  list(APPEND SOURCES
    ${BACNET_SRC}/bacapp.c
    ${BACNET_SRC}/bacdcode.c
    ${BACNET_SRC}/bacstr.c
    ${BACNET_SRC}/bacint.c
    ${BACNET_SRC}/bacreal.c
    ${BACNET_SRC}/datetime.c
    ${BACNET_SRC}/timestamp.c
    ${BACNET_SRC}/basic/sys/days.c
    ${BACNET_SRC}/bacdevobjpropref.c
    ${BACNET_SRC}/bactext.c
    ${BACNET_SRC}/indtext.c
    ${BACNET_SRC}/lighting.c
    ${BACNET_SRC}/wp.c
    ${BACNET_SRC}/cov.c
    ${BACNET_SRC}/dcc.c
    ${BACNET_SRC}/indtext.c
    ${BACNET_SRC}/lighting.c
    ${BACNET_SRC}/memcopy.c
    ${BACNET_SRC}/npdu.c
    ${BACNET_SRC}/proplist.c
    ${BACNET_SRC}/reject.c
    ${BACNET_SRC}/abort.c
    ${BACNET_SRC}/bacaddr.c
    ${BACNET_SRC}/bactimevalue.c
    ${BACNET_SRC}/bacerror.c
    ${BACNET_SRC}/basic/binding/address.c
    ${BACNET_SRC}/basic/object/acc.c
    ${BACNET_SRC}/basic/object/ai.c
    ${BACNET_SRC}/basic/object/ao.c
    ${BACNET_SRC}/basic/object/av.c
    ${BACNET_SRC}/basic/object/bi.c
    ${BACNET_SRC}/basic/object/bo.c
    ${BACNET_SRC}/basic/object/bv.c
    ${BACNET_SRC}/basic/object/channel.c
    ${BACNET_SRC}/basic/object/color_object.c
    ${BACNET_SRC}/basic/object/color_temperature.c
    ${BACNET_SRC}/basic/object/command.c
    ${BACNET_SRC}/basic/object/csv.c
    ${BACNET_SRC}/basic/object/iv.c
    ${BACNET_SRC}/basic/object/lc.c
    ${BACNET_SRC}/basic/object/lo.c
    ${BACNET_SRC}/basic/object/lsp.c
    ${BACNET_SRC}/basic/object/ms-input.c
    ${BACNET_SRC}/basic/object/mso.c
    ${BACNET_SRC}/basic/object/msv.c
    ${BACNET_SRC}/basic/object/netport.c
    ${BACNET_SRC}/basic/object/osv.c
    ${BACNET_SRC}/basic/object/piv.c
    ${BACNET_SRC}/basic/object/schedule.c
    ${BACNET_SRC}/basic/object/trendlog.c
    ${BACNET_SRC}/hostnport.c
    ${BACNET_SRC}/basic/service/h_apdu.c
    ${BACNET_SRC}/basic/service/h_cov.c
    ${BACNET_SRC}/basic/service/h_wp.c
    ${BACNET_SRC}/basic/sys/bigend.c
    ${BACNET_SRC}/basic/sys/keylist.c
    ${BACNET_SRC}/basic/tsm/tsm.c
    ${BACNET_SRC}/datalink/bvlc.c
    ${BACNET_SRC}/dailyschedule.c
    ${BACNET_SRC}/weeklyschedule.c
    ${BACNET_SRC}/basic/sys/bigend.c
    ${BACNET_SRC}/bactimevalue.c
    )

  include($ENV{ZEPHYR_BASE}/subsys/testsuite/unittest.cmake)
  project(${BACNET_NAME})
else()
  include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
  project(${BACNET_NAME})

  target_include_directories(app PRIVATE ${BACNET_INCLUDE})
  target_sources(app PRIVATE
    ${BACNET_TEST_PATH}/src/main.c
    #${BACNET_TEST_PATH}/stubs.c
    )
endif()
