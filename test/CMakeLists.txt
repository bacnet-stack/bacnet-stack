# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
# set the project name
project(Unit_Tests)

# add definitions
add_definitions(-fprofile-arcs -ftest-coverage)

# Set the compiler options
if (NOT MSVC)
    add_compile_options(-Wall -g -O0 -W -fprofile-arcs -ftest-coverage)
    # ignore some warnings that occur during unit testing
    add_compile_options(-Wno-unused-variable -Wno-unused-function)
    add_compile_options(-Wno-sign-compare -Wno-unused-parameter)
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
  unit/bacnet/bits
  )

list(APPEND testdirs
  bacnet/abort
  bacnet/alarm_ack
  bacnet/arf
  bacnet/awf
  bacnet/bacaddr
  bacnet/bacapp
  bacnet/bacdcode
  bacnet/bacdevobjpropref
  bacnet/bacerror
  bacnet/bacint
  bacnet/bacpropstates
  bacnet/bacreal
  bacnet/bacstr
  bacnet/bactimevalue
  bacnet/cov
  bacnet/datetime
  bacnet/dcc
  bacnet/event
  bacnet/getalarm
  bacnet/getevent
  bacnet/iam
  bacnet/ihave
  bacnet/indtext
  bacnet/lighting
  bacnet/lso
  bacnet/memcopy
  bacnet/npdu
  bacnet/property
  bacnet/ptransfer
  bacnet/rd
  bacnet/reject
  bacnet/rp
  bacnet/rpm
  bacnet/timestamp
  bacnet/timesync
  bacnet/whohas
  bacnet/whois
  bacnet/wp
  bacnet/weeklyschedule
  )

# bacnet/basic/*
list(APPEND testdirs
  # basic/object/binding
  bacnet/basic/binding/address
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
  bacnet/basic/object/bacfile
  bacnet/basic/object/bi
  bacnet/basic/object/bo
  bacnet/basic/object/bv
  bacnet/basic/object/color_object
  bacnet/basic/object/color_temperature
  bacnet/basic/object/command
  bacnet/basic/object/credential_data_input
  bacnet/basic/object/device
  #bacnet/basic/object/lc		#Tests skipped, redesign to use only API
  bacnet/basic/object/lo
  bacnet/basic/object/lsp
  bacnet/basic/object/ms-input
  bacnet/basic/object/mso
  bacnet/basic/object/msv
  bacnet/basic/object/netport
  bacnet/basic/object/objects
  bacnet/basic/object/osv
  bacnet/basic/object/piv
  bacnet/basic/object/schedule
  # basic/sys
  bacnet/basic/sys/color_rgb
  bacnet/basic/sys/days
  bacnet/basic/sys/fifo
  bacnet/basic/sys/filename
  bacnet/basic/sys/keylist
  bacnet/basic/sys/ringbuf
  bacnet/basic/sys/sbuf
  )

# bacnet/datalink/*
list(APPEND testdirs
  bacnet/datalink/cobs
  bacnet/datalink/crc
  bacnet/datalink/bvlc
  bacnet/datalink/bvlc-sc
  bacnet/datalink/websockets
  bacnet/datalink/bsc-socket
  bacnet/datalink/hub-sc
  bacnet/datalink/bsc-node
  bacnet/datalink/bsc-datalink
  )

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
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
