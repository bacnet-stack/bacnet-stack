# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.10 FATAL_ERROR)

get_filename_component(basename ${CMAKE_CURRENT_SOURCE_DIR} NAME)
project(test_${basename}
    VERSION 1.0.0
    LANGUAGES C)

string(REGEX REPLACE
    "/test/bacnet/[a-zA-Z0-9_/-]*$"
    "/src"
    SRC_DIR
    ${CMAKE_CURRENT_SOURCE_DIR})
string(REGEX REPLACE
    "/test/bacnet/[a-zA-Z0-9_/-]*$"
    "/test"
    TST_DIR
    ${CMAKE_CURRENT_SOURCE_DIR})

set(ZTST_DIR "${TST_DIR}/ztest/src")

add_compile_definitions(
    BACNET_BIG_ENDIAN=0
    CONFIG_ZTEST=1
    BACDL_NONE=1
    )

include_directories(
    ${SRC_DIR}
    ${TST_DIR}/ztest/include
    )

set(TEST_H_ARF_SOURCES
    # File(s) under test
    ${SRC_DIR}/bacnet/basic/service/h_arf.c
    # Support files and stubs (pathname alphabetical)
    ${SRC_DIR}/bacnet/abort.c
    ${SRC_DIR}/bacnet/arf.c
    ${SRC_DIR}/bacnet/bacaddr.c
    ${SRC_DIR}/bacnet/bacerror.c
    ${SRC_DIR}/bacnet/bacdcode.c
    ${SRC_DIR}/bacnet/bacint.c
    ${SRC_DIR}/bacnet/bacreal.c
    ${SRC_DIR}/bacnet/bacstr.c
    ${SRC_DIR}/bacnet/bactext.c
    ${SRC_DIR}/bacnet/basic/sys/bigend.c
    ${SRC_DIR}/bacnet/indtext.c
    ${SRC_DIR}/bacnet/basic/sys/debug.c
    ${SRC_DIR}/bacnet/basic/tsm/tsm.c
    ${SRC_DIR}/bacnet/datalink/datalink.c
    ${SRC_DIR}/bacnet/npdu.c
    ${SRC_DIR}/bacnet/reject.c
    # Test and test library files
    ./src/bacfile_stub.c
    ./src/main.c
    ${TST_DIR}/bacnet/basic/object/test/apdu_mock.c
    ${ZTST_DIR}/ztest_mock.c
    ${ZTST_DIR}/ztest.c
    )

add_executable(${PROJECT_NAME} ${TEST_H_ARF_SOURCES})

# Backup/Restore test binary.
add_executable(${PROJECT_NAME}_backup_restore
    ${TEST_H_ARF_SOURCES}
    ./src/device_stub.c
    )
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
