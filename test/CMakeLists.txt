# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
# set the project name
project(Unit_Tests)

option(BACNET_STACK_DEPRECATED_DISABLE "Disable deprecation compile warnings" ON)

if(BACNET_STACK_DEPRECATED_DISABLE)
  add_definitions(-DBACNET_STACK_DEPRECATED_DISABLE)
endif()

# In tests allow newer C standard than in the library.
set(CMAKE_C_STANDARD 99)

# Set the compiler options
if (CMAKE_C_COMPILER_ID MATCHES "Clang" OR CMAKE_C_COMPILER_ID MATCHES "GNU")
    add_compile_options(-g -O0 -fprofile-arcs -ftest-coverage)
    # enable all relevant warnings that find bugs
    add_compile_options(-Wall -Wextra -pedantic)
    add_compile_options(-Wfloat-equal -Wconversion)
    add_compile_options(-Wfloat-conversion -Wdouble-promotion)
    add_compile_options(-Wredundant-decls -Wmissing-declarations)
    add_compile_options(-Wswitch-default)
    add_compile_options(-Wunused-variable)
    add_compile_options(-Wcast-qual)
    add_compile_options(-Wwrite-strings)

    # don't warn about conversion, sign, compares, long long and attributes
    # since they are common in embedded
    add_compile_options(-Wno-sign-conversion -Wno-conversion)
    add_compile_options(-Wno-sign-compare)
    add_compile_options(-Wno-implicit-fallthrough)

    # Just noise from clang
    if (CMAKE_C_COMPILER_ID MATCHES "Clang" OR CMAKE_C_COMPILER_ID MATCHES "AppleClang")
      add_compile_options(-Wno-gnu-zero-variadic-macro-arguments)
    endif()

    # Should be fixed in the future
    add_compile_options(-Wno-cast-qual)
    add_compile_options(-Wno-double-promotion)
    add_compile_options(-Wno-float-conversion)
    add_compile_options(-Wno-missing-declarations)
    add_compile_options(-Wno-unused-but-set-variable)
    add_compile_options(-Wno-write-strings)
    add_compile_options(-Wno-implicit-fallthrough)

    add_link_options(-fprofile-arcs -ftest-coverage)
endif()

# Create the lcov target. Run result tests with 'make lcov'
add_custom_target(lcov
    COMMAND mkdir -p lcoverage
    )
add_custom_command(TARGET lcov
    COMMAND echo "=================== LCOV ===================="
    COMMAND echo "-- Passing lcov tool under code coverage"
    COMMAND lcov --capture --directory ../ --output-file lcoverage/main_coverage.info
    COMMAND echo "-- Generating HTML output files"
    COMMAND genhtml lcoverage/main_coverage.info --output-directory lcoverage
    )

#
# add tests
#

list(APPEND testdirs
  # unit/bacnet/bacerror
  unit/bacnet/bits
  )

list(APPEND testdirs
  bacnet/abort
  bacnet/access_rule
  bacnet/alarm_ack
  bacnet/arf
  bacnet/awf
  bacnet/bacaddr
  bacnet/bacapp
  bacnet/bacaudit
  bacnet/bacdcode
  bacnet/bacdevobjpropref
  bacnet/bacdest
  bacnet/bacerror
  bacnet/bacint
  bacnet/baclog
  bacnet/bacpropstates
  bacnet/bacreal
  bacnet/bacstr
  bacnet/bactimevalue
  bacnet/channel_value
  bacnet/cov
  bacnet/create_object
  bacnet/datetime
  bacnet/dcc
  bacnet/delete_object
  bacnet/event
  bacnet/getalarm
  bacnet/getevent
  bacnet/hostnport
  bacnet/iam
  bacnet/ihave
  bacnet/indtext
  bacnet/lighting
  bacnet/list_element
  bacnet/lso
  bacnet/memcopy
  bacnet/npdu
  bacnet/property
  bacnet/ptransfer
  bacnet/rd
  bacnet/readrange
  bacnet/reject
  bacnet/rp
  bacnet/rpm
  bacnet/secure_connect
  bacnet/specialevent
  bacnet/timestamp
  bacnet/timesync
  bacnet/weeklyschedule
  bacnet/whoami
  bacnet/whohas
  bacnet/whois
  bacnet/wp
  bacnet/wpm
  bacnet/write_group
  bacnet/youare
  )

# bacnet/basic/*
list(APPEND testdirs
  bacnet/basic/binding/address
  bacnet/basic/bbmd
  bacnet/basic/bbmd6
  bacnet/basic/bzll
  # basic/object
  bacnet/basic/object/acc
  bacnet/basic/object/access_credential
  bacnet/basic/object/access_door
  bacnet/basic/object/access_point
  bacnet/basic/object/access_rights
  bacnet/basic/object/access_user
  bacnet/basic/object/access_zone
  bacnet/basic/object/ai
  bacnet/basic/object/ao
  bacnet/basic/object/av
  bacnet/basic/object/auditlog
  bacnet/basic/object/bacfile
  bacnet/basic/object/bi
  bacnet/basic/object/bitstring_value
  bacnet/basic/object/blo
  bacnet/basic/object/bo
  bacnet/basic/object/bv
  bacnet/basic/object/channel
  bacnet/basic/object/calendar
  bacnet/basic/object/color_object
  bacnet/basic/object/color_temperature
  bacnet/basic/object/command
  bacnet/basic/object/credential_data_input
  bacnet/basic/object/csv
  bacnet/basic/object/device
  bacnet/basic/object/iv
  bacnet/basic/object/lc
  bacnet/basic/object/lo
  bacnet/basic/object/lsp
  bacnet/basic/object/lsz
  bacnet/basic/object/ms-input
  bacnet/basic/object/mso
  bacnet/basic/object/msv
  bacnet/basic/object/netport
  bacnet/basic/object/program
  bacnet/basic/object/nc
  bacnet/basic/object/objects
  bacnet/basic/object/osv
  bacnet/basic/object/piv
  bacnet/basic/object/schedule
  bacnet/basic/object/structured_view
  bacnet/basic/object/time_value
  bacnet/basic/object/trendlog
  # basic/program
  bacnet/basic/program/ubasic
  # basic/sys
  bacnet/basic/sys/color_rgb
  bacnet/basic/sys/days
  bacnet/basic/sys/dst
  bacnet/basic/sys/lighting_command
  bacnet/basic/sys/fifo
  bacnet/basic/sys/filename
  bacnet/basic/sys/keylist
  bacnet/basic/sys/linear
  bacnet/basic/sys/ringbuf
  bacnet/basic/sys/sbuf
  )

# bacnet/datalink/*
list(APPEND testdirs
  bacnet/datalink/automac
  bacnet/datalink/cobs
  bacnet/datalink/crc
  bacnet/datalink/bvlc
  bacnet/datalink/mstp
  bacnet/datalink/dlmstp
  bacnet/datalink/bvlc-sc
  )

if(BACDL_BSC)
message(STATUS "Added BACnet/SC websocket dependent tests")
list(APPEND testdirs
  bacnet/datalink/bsc-datalink
  bacnet/datalink/bsc-node
  bacnet/datalink/bsc-socket
  bacnet/datalink/hub-sc
  bacnet/datalink/websockets
)
endif()

# ports tests
if(ZEPHYR_BASE)
  message(FATAL_ERROR "ZEPHYR_BASE env variable defined.")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  message(STATUS "Added ports specific tests for linux")

  list(APPEND testdirs
  ports/linux/bsc_event
  )

elseif(WIN32)
  message(STATUS "Added ports specific tests for win32")

  list(APPEND testdirs
  ports/win32/bsc_event
  )

elseif(APPLE)
  message(STATUS "Added ports specific tests for APPLE")
  list(APPEND testdirs
  ports/bsd/bsc_event
  )
endif()

enable_testing()
foreach(testdir IN ITEMS ${testdirs})
  get_filename_component(basename ${testdir} NAME)
  add_subdirectory(${testdir})
  add_test(build_${basename}
    "${CMAKE_COMMAND}"
    --build "${CMAKE_BINARY_DIR}"
    --config "$<CONFIG>"
    --target test_${basename}
    )
  add_test(test_${basename} ${testdir}/test_${basename})
  set_tests_properties(test_${basename}  PROPERTIES FIXTURES_REQUIRED test_fixture)
  set_tests_properties(build_${basename} PROPERTIES FIXTURES_SETUP    test_fixture)
endforeach()

message(STATUS "BACNET: using cmake:....................\"${CMAKE_VERSION}\"")
message(STATUS "BACNET: CMAKE_C_COMPILER_ID:............\"${CMAKE_C_COMPILER_ID}\"")
message(STATUS "BACNET: CMAKE_C_COMPILER_VERSION:.......\"${CMAKE_C_COMPILER_VERSION}\"")
message(STATUS "BACNET: CMAKE_CXX_COMPILER_ID:..........\"${CMAKE_CXX_COMPILER_ID}\"")
message(STATUS "BACNET: CMAKE_CXX_COMPILER_VERSION:.....\"${CMAKE_CXX_COMPILER_VERSION}\"")
message(STATUS "BACNET: CMAKE_INSTALL_PREFIX:...........\"${CMAKE_INSTALL_PREFIX}\"")
