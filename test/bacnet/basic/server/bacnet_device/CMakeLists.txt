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
    BACNET_BIG_ENDIAN=0
    CONFIG_ZTEST=1
    )

include_directories(
    ${SRC_DIR}
    ${TST_DIR}/bacnet/basic/object/test
    ${TST_DIR}/ztest/include
    )

set(TEST_BACNET_DEVICE_SOURCES
    # File(s) under test
    ${SRC_DIR}/bacnet/basic/server/bacnet_device.c
    # Support files and stubs (pathname alphabetical)
    ${SRC_DIR}/bacnet/abort.c
    ${SRC_DIR}/bacnet/access_rule.c
    ${SRC_DIR}/bacnet/authentication_factor.c
    ${SRC_DIR}/bacnet/authentication_factor_format.c
    ${SRC_DIR}/bacnet/arf.c
    ${SRC_DIR}/bacnet/bacaction.c
    ${SRC_DIR}/bacnet/bacaddr.c
    ${SRC_DIR}/bacnet/bacapp.c
    ${SRC_DIR}/bacnet/bacaudit.c
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
    ${SRC_DIR}/bacnet/basic/binding/address.c
    ${SRC_DIR}/bacnet/basic/object/acc.c
    ${SRC_DIR}/bacnet/basic/object/ai.c
    ${SRC_DIR}/bacnet/basic/object/ao.c
    ${SRC_DIR}/bacnet/basic/object/av.c
    ${SRC_DIR}/bacnet/basic/object/averaging.c
    ${SRC_DIR}/bacnet/basic/object/auditlog.c
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
    ${SRC_DIR}/bacnet/basic/object/bacfile.c
    ${SRC_DIR}/bacnet/basic/object/iv.c
    ${SRC_DIR}/bacnet/basic/object/lc.c
    ${SRC_DIR}/bacnet/basic/object/lo.c
    ${SRC_DIR}/bacnet/basic/object/loop.c
    ${SRC_DIR}/bacnet/basic/object/lsp.c
    ${SRC_DIR}/bacnet/basic/object/lsz.c
    ${SRC_DIR}/bacnet/basic/object/ms-input.c
    ${SRC_DIR}/bacnet/basic/object/mso.c
    ${SRC_DIR}/bacnet/basic/object/msv.c
    ${SRC_DIR}/bacnet/basic/object/netport.c
    ${SRC_DIR}/bacnet/basic/object/osv.c
    ${SRC_DIR}/bacnet/basic/object/piv.c
    ${SRC_DIR}/bacnet/basic/object/program.c
    ${SRC_DIR}/bacnet/basic/object/schedule.c
    ${SRC_DIR}/bacnet/basic/object/structured_view.c
    ${SRC_DIR}/bacnet/basic/object/time_value.c
    ${SRC_DIR}/bacnet/basic/object/timer.c
    ${SRC_DIR}/bacnet/basic/object/trendlog.c
    ${SRC_DIR}/bacnet/basic/sys/bigend.c
    ${SRC_DIR}/bacnet/basic/sys/days.c
    ${SRC_DIR}/bacnet/basic/sys/debug.c
    ${SRC_DIR}/bacnet/basic/sys/keylist.c
    ${SRC_DIR}/bacnet/basic/sys/lighting_command.c
    ${SRC_DIR}/bacnet/basic/sys/linear.c
    ${SRC_DIR}/bacnet/basic/sys/state_name.c
    ${SRC_DIR}/bacnet/datalink/bvlc.c
    ${SRC_DIR}/bacnet/datalink/bvlc6.c
    ${SRC_DIR}/bacnet/cov.c
    ${SRC_DIR}/bacnet/create_object.c
    ${SRC_DIR}/bacnet/datetime.c
    ${SRC_DIR}/bacnet/delete_object.c
    ${SRC_DIR}/bacnet/dcc.c
    ${SRC_DIR}/bacnet/indtext.c
    ${SRC_DIR}/bacnet/hostnport.c
    ${SRC_DIR}/bacnet/lighting.c
    ${SRC_DIR}/bacnet/memcopy.c
    ${SRC_DIR}/bacnet/npdu.c
    ${SRC_DIR}/bacnet/proplist.c
    ${SRC_DIR}/bacnet/property.c
    ${SRC_DIR}/bacnet/reject.c
    ${SRC_DIR}/bacnet/shed_level.c
    ${SRC_DIR}/bacnet/timer_value.c
    ${SRC_DIR}/bacnet/timestamp.c
    ${SRC_DIR}/bacnet/wp.c
    ${SRC_DIR}/bacnet/weeklyschedule.c
    ${SRC_DIR}/bacnet/dailyschedule.c
    ${SRC_DIR}/bacnet/calendar_entry.c
    ${SRC_DIR}/bacnet/special_event.c
    ${SRC_DIR}/bacnet/channel_value.c
    ${SRC_DIR}/bacnet/secure_connect.c
    # Test and test library files
    ./src/main.c
    ${TST_DIR}/bacnet/basic/object/test/apdu_mock.c
    ${TST_DIR}/bacnet/basic/object/test/bip_mock.c
    ${TST_DIR}/bacnet/basic/object/test/cov_mock.c
    ${TST_DIR}/bacnet/basic/object/test/property_test.c
    ${TST_DIR}/bacnet/basic/object/test/datetime_local.c
    ${TST_DIR}/bacnet/basic/object/test/tsm_mock.c
    ${ZTST_DIR}/ztest_mock.c
    ${ZTST_DIR}/ztest.c
    )

add_executable(${PROJECT_NAME} ${TEST_BACNET_DEVICE_SOURCES})

# Backup/Restore test binary.
add_executable(${PROJECT_NAME}_backup_restore ${TEST_BACNET_DEVICE_SOURCES})
target_compile_definitions(${PROJECT_NAME}_backup_restore PRIVATE
    BACNET_BACKUP_RESTORE
    )
add_test(NAME build_${basename}_backup_restore
    COMMAND ${CMAKE_COMMAND} --build "${CMAKE_BINARY_DIR}" --config $<CONFIG>
            --target ${PROJECT_NAME}_backup_restore)
add_test(NAME test_${basename}_backup_restore
    COMMAND $<TARGET_FILE:${PROJECT_NAME}_backup_restore>)
set_tests_properties(test_${basename}_backup_restore PROPERTIES
    FIXTURES_REQUIRED fixture_${basename}_backup_restore)
set_tests_properties(build_${basename}_backup_restore PROPERTIES
    FIXTURES_SETUP fixture_${basename}_backup_restore)
