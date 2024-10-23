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

add_executable(${PROJECT_NAME}
    # File(s) under test
    ${SRC_DIR}/bacnet/basic/object/device.c
    # Support files and stubs (pathname alphabetical)
    ${SRC_DIR}/bacnet/abort.c
    ${SRC_DIR}/bacnet/access_rule.c
    ${SRC_DIR}/bacnet/bacaction.c
    ${SRC_DIR}/bacnet/bacaddr.c
    ${SRC_DIR}/bacnet/bacapp.c
    ${SRC_DIR}/bacnet/bacdcode.c
    ${SRC_DIR}/bacnet/bacdest.c
    ${SRC_DIR}/bacnet/bacdevobjpropref.c
    ${SRC_DIR}/bacnet/bacerror.c
    ${SRC_DIR}/bacnet/bacint.c
    ${SRC_DIR}/bacnet/bacreal.c
    ${SRC_DIR}/bacnet/bacstr.c
    ${SRC_DIR}/bacnet/bactext.c
    ${SRC_DIR}/bacnet/bactimevalue.c
    ${SRC_DIR}/bacnet/basic/binding/address.c
    ${SRC_DIR}/bacnet/basic/object/acc.c
    ${SRC_DIR}/bacnet/basic/object/ai.c
    ${SRC_DIR}/bacnet/basic/object/ao.c
    ${SRC_DIR}/bacnet/basic/object/av.c
    ${SRC_DIR}/bacnet/basic/object/bi.c
    ${SRC_DIR}/bacnet/basic/object/bitstring_value.c
    ${SRC_DIR}/bacnet/basic/object/blo.c
    ${SRC_DIR}/bacnet/basic/object/bo.c
    ${SRC_DIR}/bacnet/basic/object/bv.c
    ${SRC_DIR}/bacnet/basic/object/calendar.c
    ${SRC_DIR}/bacnet/basic/object/channel.c
    ${SRC_DIR}/bacnet/basic/object/color_object.c
    ${SRC_DIR}/bacnet/basic/object/color_temperature.c
    ${SRC_DIR}/bacnet/basic/object/command.c
    ${SRC_DIR}/bacnet/basic/object/csv.c
    ${SRC_DIR}/bacnet/basic/object/iv.c
    ${SRC_DIR}/bacnet/basic/object/lc.c
    ${SRC_DIR}/bacnet/basic/object/lo.c
    ${SRC_DIR}/bacnet/basic/object/lsp.c
    ${SRC_DIR}/bacnet/basic/object/lsz.c
    ${SRC_DIR}/bacnet/basic/object/ms-input.c
    ${SRC_DIR}/bacnet/basic/object/mso.c
    ${SRC_DIR}/bacnet/basic/object/msv.c
    ${SRC_DIR}/bacnet/basic/object/netport.c
    ${SRC_DIR}/bacnet/basic/object/osv.c
    ${SRC_DIR}/bacnet/basic/object/piv.c
    ${SRC_DIR}/bacnet/basic/object/schedule.c
    ${SRC_DIR}/bacnet/basic/object/structured_view.c
    ${SRC_DIR}/bacnet/basic/object/time_value.c
    ${SRC_DIR}/bacnet/basic/object/trendlog.c
    ${SRC_DIR}/bacnet/basic/service/h_apdu.c
    ${SRC_DIR}/bacnet/basic/service/h_cov.c
    ${SRC_DIR}/bacnet/basic/service/h_wp.c
    ${SRC_DIR}/bacnet/basic/sys/bigend.c
    ${SRC_DIR}/bacnet/basic/sys/debug.c
    ${SRC_DIR}/bacnet/basic/sys/keylist.c
    ${SRC_DIR}/bacnet/basic/sys/linear.c
    ${SRC_DIR}/bacnet/basic/tsm/tsm.c
    ${SRC_DIR}/bacnet/datalink/bvlc.c
    ${SRC_DIR}/bacnet/cov.c
    ${SRC_DIR}/bacnet/datetime.c
    ${SRC_DIR}/bacnet/basic/sys/days.c
    ${SRC_DIR}/bacnet/dcc.c
    ${SRC_DIR}/bacnet/indtext.c
    ${SRC_DIR}/bacnet/hostnport.c
    ${SRC_DIR}/bacnet/lighting.c
    ${SRC_DIR}/bacnet/memcopy.c
    ${SRC_DIR}/bacnet/npdu.c
    ${SRC_DIR}/bacnet/proplist.c
    ${SRC_DIR}/bacnet/property.c
    ${SRC_DIR}/bacnet/reject.c
    ${SRC_DIR}/bacnet/timestamp.c
    ${SRC_DIR}/bacnet/wp.c
    ${SRC_DIR}/bacnet/weeklyschedule.c
    ${SRC_DIR}/bacnet/dailyschedule.c
    ${SRC_DIR}/bacnet/calendar_entry.c
    ${SRC_DIR}/bacnet/special_event.c
    ./stubs.c
    # Test and test library files
    ./src/main.c
    ${ZTST_DIR}/ztest_mock.c
    ${ZTST_DIR}/ztest.c
    )
