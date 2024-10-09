cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

project(
  bacnet-stack
  VERSION 1.4.0
  LANGUAGES C)

#
# options
#
# <INSERT_CMAKE_INSTALL_PREFIX_HERE>

# add_compile_options(-DDEBUG_PRINT)
option(
  BACNET_STACK_BUILD_APPS
  "build apps"
  ON)

option(
  BAC_ROUTING
  "enable bac routing"
  OFF)

option(
  BACNET_PROPERTY_LISTS
  "enable property lists"
  ON)

option(
  BACNET_PROPERTY_ARRAY_LISTS
  "enable property array lists"
  ON)

option(
  BACNET_BUILD_PIFACE_APP
  "compile the piface app"
  OFF)

option(
  BACDL_ALL
  "compile with all datalink support"
  OFF)

option(
  BACNET_BUILD_BACPOLL_APP
  "compile the bacpoll app"
  ON)

option(
  BACNET_BUILD_BACDISCOVER_APP
  "compile the bacdiscover app"
  ON)

option(
  BACDL_ETHERNET
  "compile with ethernet datalink support"
  OFF)

option(
  BACDL_MSTP
  "compile with mstp datalink support"
  ON)

option(
  BACDL_ARCNET
  "compile with arcnet datalink support"
  OFF)

option(
  BACDL_BIP
  "compile with ip datalink support"
  ON)

option(
  BACDL_BIP6
  "compile with ipv6 datalink support"
  ON)

option(
  UCI
  "build with uci"
  OFF)


if(NOT (BACDL_ETHERNET OR
        BACDL_MSTP OR
        BACDL_ARCNET OR
        BACDL_BIP OR
        BACDL_BIP6 OR
        BACDL_CUSTOM))
  add_definitions(-DBACDL_NONE)
endif()

set(BACNET_PROTOCOL_REVISION 24)
set(BUILD_SHARED_LIBS 1)
# set(BACDL_ALL ON)
# set(BACDL_ARCNET ON)
set(BACDL_BIP ON)
set(BACDL_BIP6 ON)
set(BACDL_MSTP ON)
# set(BACDL_ETHERNET ON)
if(APPLE)
#  set(BACDL_MSTP OFF)
#  set(BACDL_ARCNET OFF)
#  set(BACDL_ETHERNET OFF)
set(BACDL_BIP6 OFF)
#  set(BACDL_ALL OFF)
elseif(WIN32)
#  set(BACDL_ETHERNET OFF)
set(BACDL_BIP6 OFF)
#  set(BACDL_ALL OFF)
else()
#  set(BACDL_MSTP ON)
endif()

set(BAC_ROUTING OFF)
set(BACNET_STACK_BUILD_APPS OFF)
#set(BACNET_VENDOR_NAME "@Schneider Electric@" CACHE STRINGi "Vendor ID")
# set(OPT2 "Default" CACHE STRING "Helpstring")
# set(CMAKE_INSTALL_PREFIX "/usr/")

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# Build types
if ("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
    message(STATUS "Maximum optimization for speed")
elseif ("${CMAKE_BUILD_TYPE}" STREQUAL "RelWithDebInfo")
    message(STATUS "Maximum optimization for speed, debug info included")
elseif ("${CMAKE_BUILD_TYPE}" STREQUAL "MinSizeRel")
    message(STATUS "Maximum optimization for size")
else ()
    message(STATUS "Minimal optimization, debug info included")
    add_compile_definitions(DEBUG_ENABLED=0)
endif ()

option(
  BACNET_STACK_DEPRECATED_DISABLE
  "Disable deprecation compile warnings"
  ON)

if(BACNET_STACK_DEPRECATED_DISABLE)
  add_definitions(-DBACNET_STACK_DEPRECATED_DISABLE)
endif()

set(CMAKE_C_STANDARD 90)

if (CMAKE_C_COMPILER_ID MATCHES "Clang" OR CMAKE_C_COMPILER_ID MATCHES "AppleClang" OR CMAKE_C_COMPILER_ID MATCHES "GNU")
  add_compile_options(-Wall -Wextra -pedantic)
  # Add more warnings
  add_compile_options(-Wfloat-equal -Wconversion)
  add_compile_options(-Wfloat-conversion -Wdouble-promotion)
  add_compile_options(-Wredundant-decls -Wmissing-declarations)
  add_compile_options(-Wswitch-default)
  add_compile_options(-Wunused-variable)
  add_compile_options(-Wcast-qual)
  add_compile_options(-Wwrite-strings)

  # Don't warn about conversion, sign, compares since they are common in
  # embedded
  add_compile_options(-Wno-sign-conversion -Wno-conversion)
  add_compile_options(-Wno-sign-compare)

  if (CMAKE_C_COMPILER_ID MATCHES "Clang" OR CMAKE_C_COMPILER_ID MATCHES "AppleClang")
    # Just noise from clang
    add_compile_options(-Wno-gnu-zero-variadic-macro-arguments)

    # Clang does not like _Bool which is not C90. We could fix this at some
    # point.
    add_compile_options(-Wno-c99-extensions)
  endif()

  if (APPLE)
    # Apple defines UINT64_MAX with ULL suffix. This is not C90 compliant.
    add_compile_options(-Wno-long-long)
  endif()

  # Should be fixed in a future.
  add_compile_options(-Wno-cast-qual)
  add_compile_options(-Wno-double-promotion)
  add_compile_options(-Wno-float-conversion)
  add_compile_options(-Wno-missing-declarations)
  add_compile_options(-Wno-unused-but-set-variable)
  add_compile_options(-Wno-write-strings)
  add_compile_options(-Wno-language-extension-token)
elseif(MSVC)
  add_compile_options(/Wall)

  # Potentially uninitialized local variable. Definetly fix these. There might
  # be false positives but it is better to fix them also.
  add_compile_options(/wd4701)

  # Function is unsafe (strtok, sscanf, fopen, strerror).
  add_compile_options(/wd4996)

  # Handle all enums values in switch.
  add_compile_options(/wd4061)

  # Possible loss of data.
  add_compile_options(/wd4242 /wd4244 /wd4267)

  # Signed/unsigned mismatch.
  add_compile_options(/wd4018 /wd4388 /wd4389)

  # Not defined as a preprocessor macro.
  add_compile_options(/wd4668)

  # Function not inlined.
  add_compile_options(/wd4710)

  # Compiler adds padding to structures.
  add_compile_options(/wd4820)

  # Might be slower if builded with /Qspectre
  add_compile_options(/wd5045)
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "AppleClang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  add_compile_options(-Wall -Wextra -pedantic -g -O3)
endif()

#
# library
#

find_package(Threads)

if(UCI)
  FIND_PATH(uci_include_dir uci.h)
  FIND_LIBRARY(uci_lib NAMES uci)
  if (uci_include_dir AND uci_lib)
    message(STATUS "BACNET: BAC_UCI Include:................\"${uci_include_dir}\"")
    message(STATUS "BACNET: BAC_UCI Lib:....................\"${uci_lib}\"")
    include_directories(${uci_include_dir})
    link_libraries(${uci_lib})
    add_compile_definitions(BAC_UCI)
  else()
    message(WARNING "BACNET: BAC_UCI Lib:....................\"not found\"")
    set(UCI OFF)
  endif()
endif()

add_library(${PROJECT_NAME}
  src/bacnet/abort.c
  src/bacnet/abort.h
  src/bacnet/access_rule.c
  src/bacnet/access_rule.h
  src/bacnet/alarm_ack.c
  src/bacnet/alarm_ack.h
  src/bacnet/apdu.h
  src/bacnet/arf.c
  src/bacnet/arf.h
  src/bacnet/assigned_access_rights.c
  src/bacnet/assigned_access_rights.h
  src/bacnet/authentication_factor.c
  src/bacnet/authentication_factor_format.c
  src/bacnet/authentication_factor_format.h
  src/bacnet/authentication_factor.h
  src/bacnet/awf.c
  src/bacnet/awf.h
  src/bacnet/bacaddr.c
  src/bacnet/bacaddr.h
  src/bacnet/bacaction.c
  src/bacnet/bacaction.h
  src/bacnet/bacapp.c
  src/bacnet/bacapp.h
  src/bacnet/bacdcode.c
  src/bacnet/bacdcode.h
  src/bacnet/bacdef.h
  src/bacnet/bacdest.c
  src/bacnet/bacdest.h
  src/bacnet/bacdevobjpropref.c
  src/bacnet/bacdevobjpropref.h
  src/bacnet/bacenum.h
  src/bacnet/bacerror.c
  src/bacnet/bacerror.h
  src/bacnet/bacint.c
  src/bacnet/bacint.h
  src/bacnet/bacprop.c
  src/bacnet/bacprop.h
  src/bacnet/bacpropstates.c
  src/bacnet/bacpropstates.h
  src/bacnet/bacreal.c
  src/bacnet/bacreal.h
  src/bacnet/bacstr.c
  src/bacnet/bacstr.h
  src/bacnet/bactext.c
  src/bacnet/bactext.h
  src/bacnet/bactimevalue.c
  src/bacnet/bactimevalue.h
  src/bacnet/dailyschedule.c
  src/bacnet/dailyschedule.h
  src/bacnet/weeklyschedule.c
  src/bacnet/weeklyschedule.h
  src/bacnet/special_event.c
  src/bacnet/special_event.h
  $<$<BOOL:${BACDL_BIP}>:src/bacnet/basic/bbmd/h_bbmd.c>
  $<$<BOOL:${BACDL_BIP}>:src/bacnet/basic/bbmd/h_bbmd.h>
  $<$<BOOL:${BACDL_BIP6}>:src/bacnet/basic/bbmd6/h_bbmd6.c>
  $<$<BOOL:${BACDL_BIP6}>:src/bacnet/basic/bbmd6/h_bbmd6.h>
  $<$<BOOL:${BACDL_BIP6}>:src/bacnet/basic/bbmd6/vmac.c>
  $<$<BOOL:${BACDL_BIP6}>:src/bacnet/basic/bbmd6/vmac.h>
  src/bacnet/basic/binding/address.c
  src/bacnet/basic/binding/address.h
  src/bacnet/basic/npdu/h_npdu.c
  src/bacnet/basic/npdu/h_npdu.h
  $<$<BOOL:${BAC_ROUTING}>:src/bacnet/basic/npdu/h_routed_npdu.c>
  $<$<BOOL:${BAC_ROUTING}>:src/bacnet/basic/npdu/h_routed_npdu.h>
  src/bacnet/basic/npdu/s_router.c
  src/bacnet/basic/npdu/s_router.h
  src/bacnet/basic/object/access_credential.c
  src/bacnet/basic/object/access_credential.h
  src/bacnet/basic/object/access_door.c
  src/bacnet/basic/object/access_door.h
  src/bacnet/basic/object/access_point.c
  src/bacnet/basic/object/access_point.h
  src/bacnet/basic/object/access_rights.c
  src/bacnet/basic/object/access_rights.h
  src/bacnet/basic/object/access_user.c
  src/bacnet/basic/object/access_user.h
  src/bacnet/basic/object/access_zone.c
  src/bacnet/basic/object/access_zone.h
  src/bacnet/basic/object/acc.c
  src/bacnet/basic/object/ai.c
  src/bacnet/basic/object/ai.h
  src/bacnet/basic/object/ao.c
  src/bacnet/basic/object/ao.h
  src/bacnet/basic/object/av.c
  src/bacnet/basic/object/av.h
  src/bacnet/basic/object/bacfile.c
  src/bacnet/basic/object/bacfile.h
  src/bacnet/basic/object/bi.c
  src/bacnet/basic/object/bi.h
  src/bacnet/basic/object/bitstring_value.c
  src/bacnet/basic/object/bitstring_value.h
  src/bacnet/basic/object/blo.c
  src/bacnet/basic/object/blo.h
  src/bacnet/basic/object/bo.c
  src/bacnet/basic/object/bo.h
  src/bacnet/basic/object/bv.c
  src/bacnet/basic/object/bv.h
  src/bacnet/basic/object/channel.c
  src/bacnet/basic/object/channel.h
  src/bacnet/basic/object/calendar.c
  src/bacnet/basic/object/calendar.h
  src/bacnet/basic/object/color_object.c
  src/bacnet/basic/object/color_object.h
  src/bacnet/basic/object/color_temperature.c
  src/bacnet/basic/object/color_temperature.h
  # src/bacnet/basic/object/client/device-client.c
  src/bacnet/basic/object/command.c
  src/bacnet/basic/object/command.h
  src/bacnet/basic/object/credential_data_input.c
  src/bacnet/basic/object/credential_data_input.h
  src/bacnet/basic/object/csv.c
  src/bacnet/basic/object/csv.h
  src/bacnet/basic/object/device.c
  src/bacnet/basic/object/device.h
  $<$<BOOL:${BAC_ROUTING}>:src/bacnet/basic/object/gateway/gw_device.c>
  src/bacnet/basic/object/iv.c
  src/bacnet/basic/object/iv.h
  src/bacnet/basic/object/lc.c
  src/bacnet/basic/object/lc.h
  src/bacnet/basic/object/lo.c
  src/bacnet/basic/object/lo.h
  src/bacnet/basic/object/lsp.c
  src/bacnet/basic/object/lsp.h
  src/bacnet/basic/object/lsz.c
  src/bacnet/basic/object/lsz.h
  src/bacnet/basic/object/ms-input.c
  src/bacnet/basic/object/ms-input.h
  src/bacnet/basic/object/mso.c
  src/bacnet/basic/object/mso.h
  src/bacnet/basic/object/msv.c
  src/bacnet/basic/object/msv.h
  src/bacnet/basic/object/nc.c
  src/bacnet/basic/object/nc.h
  src/bacnet/basic/object/netport.c
  src/bacnet/basic/object/netport.h
  src/bacnet/basic/object/objects.c
  src/bacnet/basic/object/objects.h
  src/bacnet/basic/object/osv.c
  src/bacnet/basic/object/osv.h
  src/bacnet/basic/object/piv.c
  src/bacnet/basic/object/piv.h
  src/bacnet/basic/object/schedule.c
  src/bacnet/basic/object/schedule.h
  src/bacnet/basic/object/structured_view.c
  src/bacnet/basic/object/structured_view.h
  src/bacnet/basic/object/time_value.c
  src/bacnet/basic/object/time_value.h
  src/bacnet/basic/object/trendlog.c
  src/bacnet/basic/object/trendlog.h
  src/bacnet/basic/service/h_alarm_ack.c
  src/bacnet/basic/service/h_alarm_ack.h
  src/bacnet/basic/service/h_apdu.c
  src/bacnet/basic/service/h_apdu.h
  src/bacnet/basic/service/h_arf_a.c
  src/bacnet/basic/service/h_arf_a.h
  src/bacnet/basic/service/h_arf.c
  src/bacnet/basic/service/h_arf.h
  src/bacnet/basic/service/h_awf.c
  src/bacnet/basic/service/h_awf.h
  src/bacnet/basic/service/h_ccov.c
  src/bacnet/basic/service/h_ccov.h
  src/bacnet/basic/service/h_create_object.c
  src/bacnet/basic/service/h_create_object.h
  src/bacnet/basic/service/h_cov.c
  src/bacnet/basic/service/h_cov.h
  src/bacnet/basic/service/h_dcc.c
  src/bacnet/basic/service/h_dcc.h
  src/bacnet/basic/service/h_delete_object.c
  src/bacnet/basic/service/h_delete_object.h
  src/bacnet/basic/service/h_gas_a.c
  src/bacnet/basic/service/h_gas_a.h
  src/bacnet/basic/service/h_get_alarm_sum.c
  src/bacnet/basic/service/h_get_alarm_sum.h
  src/bacnet/basic/service/h_getevent_a.c
  src/bacnet/basic/service/h_getevent_a.h
  src/bacnet/basic/service/h_getevent.c
  src/bacnet/basic/service/h_getevent.h
  src/bacnet/basic/service/h_iam.c
  src/bacnet/basic/service/h_iam.h
  src/bacnet/basic/service/h_ihave.c
  src/bacnet/basic/service/h_ihave.h
  src/bacnet/basic/service/h_lso.c
  src/bacnet/basic/service/h_lso.h
  src/bacnet/basic/service/h_list_element.c
  src/bacnet/basic/service/h_list_element.h
  src/bacnet/basic/service/h_noserv.c
  src/bacnet/basic/service/h_noserv.h
  src/bacnet/basic/service/h_rd.c
  src/bacnet/basic/service/h_rd.h
  src/bacnet/basic/service/h_rp_a.c
  src/bacnet/basic/service/h_rp_a.h
  src/bacnet/basic/service/h_rp.c
  src/bacnet/basic/service/h_rp.h
  src/bacnet/basic/service/h_rpm_a.c
  src/bacnet/basic/service/h_rpm_a.h
  src/bacnet/basic/service/h_rpm.c
  src/bacnet/basic/service/h_rpm.h
  src/bacnet/basic/service/h_rr_a.c
  src/bacnet/basic/service/h_rr_a.h
  src/bacnet/basic/service/h_rr.c
  src/bacnet/basic/service/h_rr.h
  src/bacnet/basic/service/h_ts.c
  src/bacnet/basic/service/h_ts.h
  src/bacnet/basic/service/h_ucov.c
  src/bacnet/basic/service/h_ucov.h
  src/bacnet/basic/service/h_upt.c
  src/bacnet/basic/service/h_upt.h
  src/bacnet/basic/service/h_whohas.c
  src/bacnet/basic/service/h_whohas.h
  src/bacnet/basic/service/h_whois.c
  src/bacnet/basic/service/h_whois.h
  src/bacnet/basic/service/h_wp.c
  src/bacnet/basic/service/h_wp.h
  src/bacnet/basic/service/h_wpm.c
  src/bacnet/basic/service/h_wpm.h
  src/bacnet/basic/service/s_abort.c
  src/bacnet/basic/service/s_abort.h
  src/bacnet/basic/service/s_ack_alarm.c
  src/bacnet/basic/service/s_ack_alarm.h
  src/bacnet/basic/service/s_arfs.c
  src/bacnet/basic/service/s_arfs.h
  src/bacnet/basic/service/s_awfs.c
  src/bacnet/basic/service/s_awfs.h
  src/bacnet/basic/service/s_cevent.c
  src/bacnet/basic/service/s_cevent.h
  src/bacnet/basic/service/s_create_object.c
  src/bacnet/basic/service/s_create_object.h
  src/bacnet/basic/service/s_cov.c
  src/bacnet/basic/service/s_cov.h
  src/bacnet/basic/service/s_dcc.c
  src/bacnet/basic/service/s_dcc.h
  src/bacnet/basic/service/s_delete_object.c
  src/bacnet/basic/service/s_delete_object.h
  src/bacnet/basic/service/s_error.c
  src/bacnet/basic/service/s_error.h
  src/bacnet/basic/service/s_get_alarm_sum.c
  src/bacnet/basic/service/s_get_alarm_sum.h
  src/bacnet/basic/service/s_get_event.c
  src/bacnet/basic/service/s_getevent.c
  src/bacnet/basic/service/s_get_event.h
  src/bacnet/basic/service/s_getevent.h
  src/bacnet/basic/service/s_iam.c
  src/bacnet/basic/service/s_iam.h
  src/bacnet/basic/service/s_ihave.c
  src/bacnet/basic/service/s_ihave.h
  src/bacnet/basic/service/s_list_element.c
  src/bacnet/basic/service/s_list_element.h
  src/bacnet/basic/service/s_lso.c
  src/bacnet/basic/service/s_lso.h
  src/bacnet/basic/service/s_rd.c
  src/bacnet/basic/service/s_rd.h
  src/bacnet/basic/service/s_readrange.c
  src/bacnet/basic/service/s_readrange.h
  src/bacnet/basic/service/s_rp.c
  src/bacnet/basic/service/s_rp.h
  src/bacnet/basic/service/s_rpm.c
  src/bacnet/basic/service/s_rpm.h
  src/bacnet/basic/service/s_ts.c
  src/bacnet/basic/service/s_ts.h
  src/bacnet/basic/service/s_uevent.c
  src/bacnet/basic/service/s_uevent.h
  src/bacnet/basic/service/s_upt.c
  src/bacnet/basic/service/s_upt.h
  src/bacnet/basic/service/s_whohas.c
  src/bacnet/basic/service/s_whohas.h
  src/bacnet/basic/service/s_whois.c
  src/bacnet/basic/service/s_whois.h
  src/bacnet/basic/service/s_wp.c
  src/bacnet/basic/service/s_wp.h
  src/bacnet/basic/service/s_wpm.c
  src/bacnet/basic/service/s_wpm.h
  src/bacnet/basic/services.h
  src/bacnet/basic/sys/bigend.c
  src/bacnet/basic/sys/bigend.h
  src/bacnet/basic/sys/color_rgb.c
  src/bacnet/basic/sys/color_rgb.h
  src/bacnet/basic/sys/days.c
  src/bacnet/basic/sys/days.h
  src/bacnet/basic/sys/debug.c
  src/bacnet/basic/sys/debug.h
  src/bacnet/basic/sys/fifo.c
  src/bacnet/basic/sys/fifo.h
  src/bacnet/basic/sys/filename.c
  src/bacnet/basic/sys/filename.h
  src/bacnet/basic/sys/key.h
  src/bacnet/basic/sys/keylist.c
  src/bacnet/basic/sys/keylist.h
  src/bacnet/basic/sys/linear.c
  src/bacnet/basic/sys/linear.h
  src/bacnet/basic/sys/mstimer.c
  src/bacnet/basic/sys/mstimer.h
  src/bacnet/basic/sys/ringbuf.c
  src/bacnet/basic/sys/ringbuf.h
  src/bacnet/basic/sys/sbuf.c
  src/bacnet/basic/sys/sbuf.h
  src/bacnet/basic/tsm/tsm.c
  src/bacnet/basic/tsm/tsm.h
  src/bacnet/basic/sys/bits.h
  src/bacnet/basic/sys/bytes.h
  src/bacnet/config.h
  src/bacnet/calendar_entry.c
  src/bacnet/calendar_entry.h
  src/bacnet/cov.c
  src/bacnet/cov.h
  src/bacnet/create_object.c
  src/bacnet/create_object.h
  src/bacnet/credential_authentication_factor.c
  src/bacnet/credential_authentication_factor.h
  src/bacnet/datalink/arcnet.h
  src/bacnet/datalink/bacsec.c
  src/bacnet/datalink/bacsec.h
  src/bacnet/datalink/bip6.h
  $<$<BOOL:${BACDL_BIP}>:src/bacnet/datalink/bip.h>
  $<$<BOOL:${BACDL_BIP6}>:src/bacnet/datalink/bvlc6.c>
  $<$<BOOL:${BACDL_BIP6}>:src/bacnet/datalink/bvlc6.h>
  $<$<BOOL:${BACDL_BIP}>:src/bacnet/datalink/bvlc.h>
  $<$<BOOL:${BACDL_BIP}>:src/bacnet/datalink/bvlc.c>
  $<$<BOOL:${BACDL_MSTP}>:src/bacnet/datalink/crc.h>
  $<$<BOOL:${BACDL_MSTP}>:src/bacnet/datalink/crc.c>
  $<$<BOOL:${BACDL_MSTP}>:src/bacnet/datalink/cobs.h>
  $<$<BOOL:${BACDL_MSTP}>:src/bacnet/datalink/cobs.c>
  src/bacnet/datalink/datalink.c
  src/bacnet/datalink/datalink.h
  src/bacnet/datalink/dlenv.c
  src/bacnet/datalink/dlenv.h
  src/bacnet/datalink/dlmstp.h
  src/bacnet/datalink/ethernet.h
  $<$<BOOL:${BACDL_MSTP}>:src/bacnet/datalink/mstp.c>
  src/bacnet/datalink/mstpdef.h
  src/bacnet/datalink/mstp.h
  src/bacnet/datalink/mstptext.c
  src/bacnet/datalink/mstptext.h
  src/bacnet/datetime.c
  src/bacnet/datetime.h
  src/bacnet/dcc.c
  src/bacnet/dcc.h
  src/bacnet/delete_object.c
  src/bacnet/delete_object.h
  src/bacnet/event.c
  src/bacnet/event.h
  src/bacnet/get_alarm_sum.c
  src/bacnet/get_alarm_sum.h
  src/bacnet/getevent.c
  src/bacnet/getevent.h
  src/bacnet/hostnport.c
  src/bacnet/hostnport.h
  src/bacnet/iam.c
  src/bacnet/iam.h
  src/bacnet/ihave.c
  src/bacnet/ihave.h
  src/bacnet/indtext.c
  src/bacnet/indtext.h
  src/bacnet/lighting.c
  src/bacnet/lighting.h
  src/bacnet/list_element.c
  src/bacnet/list_element.h
  src/bacnet/lso.c
  src/bacnet/lso.h
  src/bacnet/memcopy.c
  src/bacnet/memcopy.h
  src/bacnet/npdu.c
  src/bacnet/npdu.h
  src/bacnet/property.c
  src/bacnet/property.h
  src/bacnet/proplist.c
  src/bacnet/proplist.h
  src/bacnet/ptransfer.c
  src/bacnet/ptransfer.h
  src/bacnet/rd.c
  src/bacnet/rd.h
  src/bacnet/readrange.c
  src/bacnet/readrange.h
  src/bacnet/reject.c
  src/bacnet/reject.h
  src/bacnet/rp.c
  src/bacnet/rp.h
  src/bacnet/rpm.c
  src/bacnet/rpm.h
  src/bacnet/timestamp.c
  src/bacnet/timestamp.h
  src/bacnet/timesync.c
  src/bacnet/timesync.h
  src/bacnet/version.h
  src/bacnet/whohas.c
  src/bacnet/whohas.h
  src/bacnet/whois.c
  src/bacnet/whois.h
  src/bacnet/wp.c
  src/bacnet/wp.h
  src/bacnet/wpm.c
  src/bacnet/wpm.h
  $<$<BOOL:${UCI}>:src/bacnet/basic/ucix/ucix.c>
  $<$<BOOL:${UCI}>:src/bacnet/basic/ucix/ucix.h>)
target_sources(
  ${PROJECT_NAME}
  PRIVATE
  ${BACNETSTACK_SRCS})
target_include_directories(
  ${PROJECT_NAME}
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/src/>
         $<INSTALL_INTERFACE:include/bacnet/>)
if(BUILD_SHARED_LIBS)
  set_target_properties(
    ${PROJECT_NAME}
    PROPERTIES
      C_VISIBILITY_PRESET hidden)
endif()

if(APPLE)
  unset(BACDL_MSTP)
endif()

target_compile_definitions(
  ${PROJECT_NAME}
  PUBLIC
  BACNET_PROTOCOL_REVISION=${BACNET_PROTOCOL_REVISION}
  BACNET_VENDOR_NAME="Schneider Electric"
  BBMD_ENABLED=0
  BBMD_CLIENT_ENABLED=1
  INTRINSIC_REPORTING=1
  BINARY_INPUT_INTRINSIC_REPORTING=1
  BINARY_VALUE_INTRINSIC_REPORTING=1
  MAX_ANALOG_VALUES=32
  MAX_ANALOG_INPUTS=128
  MAX_BINARY_VALUES=200
  MAX_BINARY_INPUTS=16
  MAX_CHARACTERSTRING_VALUES=16
  MAX_INTEGER_VALUES=12
  MAX_MULTISTATE_VALUES=32
  MULTISTATE_NUMBER_OF_STATES=64
  MAX_NOTIFICATION_CLASSES=10
  MAX_CHARACTER_STRING_BYTES=384
  # NC_MAX_RECIPIENTS=4
  $<$<BOOL:${BACDL_BIP}>:BACDL_BIP>
  $<$<BOOL:${BACDL_BIP6}>:BACDL_BIP6>
  $<$<BOOL:${BACDL_ARCNET}>:BACDL_ARCNET>
  $<$<BOOL:${BACDL_MSTP}>:BACDL_MSTP>
  $<$<BOOL:${BACDL_ETHERNET}>:BACDL_ETHERNET>
  $<$<BOOL:${BACDL_NONE}>:BACDL_NONE>
  $<$<BOOL:${BACNET_PROPERTY_LISTS}>:BACNET_PROPERTY_LISTS=1>
  $<$<BOOL:${BACNET_PROPERTY_ARRAY_LISTS}>:BACNET_PROPERTY_ARRAY_LISTS=1>
  $<$<BOOL:${BAC_ROUTING}>:BAC_ROUTING>
  # $<$<BOOL:${BACDL_ALL}>:BACDL_ALL>
  #  $<$<NOT:$<BOOL:${BAC_ROUTING}>>:BAC_ROUTING>
  $<$<NOT:$<BOOL:${BUILD_SHARED_LIBS}>>:BACNET_STACK_STATIC_DEFINE>
  PRIVATE
  BACDL_MULTIPLE=1
  CSTANDARD=-std=c17
  PRINT_ENABLED=1)

target_link_libraries(${PROJECT_NAME} PUBLIC Threads::Threads)

add_library(
  ${PROJECT_NAME}::${PROJECT_NAME}
  ALIAS
  ${PROJECT_NAME})

#
# add ports
#

set(BACNET_PORT_DIRECTORY_PATH "")

if(ZEPHYR_BASE)
  message(FATAL_ERROR "ZEPHYR_BASE env variable defined. Use zephyr/CMakeLists.txt for Zephyr build")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  message(STATUS "BACNET: building for linux")
  set(BACNET_PORT_DIRECTORY_PATH ${CMAKE_CURRENT_LIST_DIR}/ports/linux)
  target_link_libraries(${PROJECT_NAME} PUBLIC m)

  target_sources(${PROJECT_NAME} PRIVATE
    ports/linux/bacport.h
    ports/linux/datetime-init.c
    $<$<BOOL:${BACDL_BIP}>:ports/linux/bip-init.c>
    $<$<BOOL:${BACDL_BIP6}>:ports/linux/bip6.c>
    $<$<BOOL:${BACDL_ARCNET}>:ports/linux/arcnet.c>
    $<$<BOOL:${BACDL_MSTP}>:ports/linux/rs485.c>
    $<$<BOOL:${BACDL_MSTP}>:ports/linux/rs485.h>
    $<$<BOOL:${BACDL_MSTP}>:ports/linux/dlmstp.c>
    #$<$<BOOL:${BACDL_MSTP}>:ports/linux/dlmstp_linux.c>
    #$<$<BOOL:${BACDL_MSTP}>:ports/linux/dlmstp_linux.h>
    $<$<BOOL:${BACDL_ETHERNET}>:ports/linux/ethernet.c>
    ports/linux/mstimer-init.c)

elseif(WIN32)
  message(STATUS "BACNET: building for win32")
  set(BACNET_PORT_DIRECTORY_PATH ${CMAKE_CURRENT_LIST_DIR}/ports/win32)

  target_link_libraries(${PROJECT_NAME} PUBLIC wsock32)

  target_link_libraries(${PROJECT_NAME} PRIVATE
    winmm
    $<$<BOOL:${BACDL_BIP}>:ws2_32>
    $<$<BOOL:${BACDL_BIP}>:iphlpapi>)

  target_sources(${PROJECT_NAME} PRIVATE
    ports/win32/bacport.h
    $<$<BOOL:${BACDL_BIP6}>:ports/win32/bip6.c>
    $<$<BOOL:${BACDL_BIP}>:ports/win32/bip-init.c>
    ports/win32/datetime-init.c
    $<$<BOOL:${BACDL_MSTP}>:ports/win32/dlmstp.c>
    #  ports/win32/dlmstp-mm.c
    $<$<BOOL:${BACDL_ETHERNET}>:ports/win32/ethernet.c>
    ports/win32/mstimer-init.c
    $<$<BOOL:${BACDL_MSTP}>:ports/win32/rs485.c>
    $<$<BOOL:${BACDL_MSTP}>:ports/win32/rs485.h>)
elseif(APPLE)
  message(STATUS "BACNET: building for APPLE")
  set(BACNET_PORT_DIRECTORY_PATH ${CMAKE_CURRENT_LIST_DIR}/ports/bsd)
  add_compile_definitions(BACNET_PORT=bsd)

  target_sources(${PROJECT_NAME} PRIVATE
    ports/bsd/bacport.h
    ports/bsd/bip-init.c
    $<$<BOOL:${BACDL_BIP}>:ports/bsd/bip-init.c>
    $<$<BOOL:${BACDL_BIP6}>:ports/bsd/bip6.c>
    $<$<BOOL:${BACDL_MSTP}>:ports/bsd/rs485.c>
    $<$<BOOL:${BACDL_MSTP}>:ports/bsd/rs485.h>
    $<$<BOOL:${BACDL_MSTP}>:ports/bsd/dlmstp.c>
    ports/bsd/datetime-init.c
    ports/bsd/mstimer-init.c
    ports/bsd/stdbool.h)

  if(BACDL_ETHERNET)
    FIND_PATH(PCAP_INCLUDE pcap.h)
    if(NOT PCAP_INCLUDE)
      message(WARNING "BACNET: pcap.h header file not found")
    else()
      message(STATUS "BACNET: pcap Include:...................\"${PCAP_INCLUDE}\"")
    endif()
    FIND_LIBRARY(PCAP_LIBRARIES NAMES pcap)
    if(NOT PCAP_LIBRARIES)
      message(WARNING "BACNET: libpcap not found")
    else()
      message(STATUS "BACNET: pcap Lib:.......................\"${PCAP_LIBRARIES}\"")
    endif()
    if(PCAP_INCLUDE AND PCAP_LIBRARIES)
      include_directories(${PCAP_INCLUDE})
      link_libraries(${PCAP_LIBRARIES})
      target_sources(${PROJECT_NAME} PRIVATE
        ports/bsd/ethernet.c
      )
    endif()
  endif()
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD")
  message(STATUS "BACNET: building for FreeBSD")
  set(BACNET_PORT_DIRECTORY_PATH ${CMAKE_CURRENT_LIST_DIR}/ports/bsd)

  target_sources(${PROJECT_NAME} PRIVATE
    ports/bsd/bacport.h
    ports/bsd/bip-init.c
    ports/bsd/datetime-init.c
    ports/bsd/mstimer-init.c
    ports/bsd/stdbool.h)
endif()

target_include_directories(${PROJECT_NAME} PUBLIC
  $<BUILD_INTERFACE:${BACNET_PORT_DIRECTORY_PATH}>
)

#
# examples
#

if(BACNET_STACK_BUILD_APPS)
  message(STATUS "BACNET: compiling also apps")

  add_executable(abort apps/abort/main.c)
  target_link_libraries(abort PRIVATE ${PROJECT_NAME})

  add_executable(ack-alarm apps/ack-alarm/main.c)
  target_link_libraries(ack-alarm PRIVATE ${PROJECT_NAME})

  add_executable(add-list-element apps/add-list-element/main.c)
  target_link_libraries(add-list-element PRIVATE ${PROJECT_NAME})

  add_executable(apdu apps/apdu/main.c)
  target_link_libraries(apdu PRIVATE ${PROJECT_NAME})

  add_executable(create-object apps/create-object/main.c)
  target_link_libraries(create-object PRIVATE ${PROJECT_NAME})

  add_executable(dcc apps/dcc/main.c)
  target_link_libraries(dcc PRIVATE ${PROJECT_NAME})

  add_executable(delete-object apps/delete-object/main.c)
  target_link_libraries(delete-object PRIVATE ${PROJECT_NAME})

  add_executable(epics apps/epics/main.c)
  target_link_libraries(epics PRIVATE ${PROJECT_NAME})

  add_executable(error apps/error/main.c)
  target_link_libraries(error PRIVATE ${PROJECT_NAME})

  # add_executable(gateway apps/gateway/main.c apps/gateway/gateway.h)
  # target_link_libraries(gateway PRIVATE ${PROJECT_NAME})

  add_executable(getevent apps/getevent/main.c)
  target_link_libraries(getevent PRIVATE ${PROJECT_NAME})

  add_executable(iam apps/iam/main.c)
  target_link_libraries(iam PRIVATE ${PROJECT_NAME})

  add_executable(iamrouter apps/iamrouter/main.c)
  target_link_libraries(iamrouter PRIVATE ${PROJECT_NAME})

  add_executable(initrouter apps/initrouter/main.c)
  target_link_libraries(initrouter PRIVATE ${PROJECT_NAME})

  if(BACDL_MSTP)
    add_executable(mstpcap apps/mstpcap/main.c)
    target_link_libraries(mstpcap PRIVATE ${PROJECT_NAME})
    target_compile_options(mstpcap PRIVATE
      # NOTE: Might be that this example currently doesn't work on Windows because
      # of the following warning:

      # 'strncasecmp': macro redefinition
      $<$<C_COMPILER_ID:MSVC>:/wd4005>
      # 'gettimeofday' undefined; assuming extern returning int
      $<$<C_COMPILER_ID:MSVC>:/wd4013>
    )

    add_executable(mstpcrc apps/mstpcrc/main.c)
    target_link_libraries(mstpcrc PRIVATE ${PROJECT_NAME})
    target_compile_options(mstpcrc PRIVATE
      # NOTE: Might be that this example currently doesn't work on Windows because
      # of the following warning:
      # 'gettimeofday' undefined; assuming extern returning int
      $<$<C_COMPILER_ID:MSVC>:/wd4013>
    )
  endif()

  if(BACNET_BUILD_PIFACE_APP)
    add_executable(piface apps/piface/main.c apps/piface/device.c)
    target_link_libraries(piface PRIVATE ${PROJECT_NAME})
  endif(BACNET_BUILD_PIFACE_APP)

  if(BACNET_BUILD_BACPOLL_APP)
    add_executable(bacpoll
      apps/server-client/main.c
      src/bacnet/basic/client/bac-task.c
      src/bacnet/basic/client/bac-data.c
      src/bacnet/basic/client/bac-rw.c)
    target_link_libraries(bacpoll PRIVATE ${PROJECT_NAME})
    target_compile_options(bacpoll PRIVATE
      # Unreachable code because we have endless loop.
      $<$<C_COMPILER_ID:MSVC>:/wd4702>
    )
  endif(BACNET_BUILD_BACPOLL_APP)

  if(NOT BACDL_ETHERNET)
    if(BAC_ROUTING)
  if(BACNET_BUILD_BACDISCOVER_APP)
    add_executable(bacdiscover
      apps/server-discover/main.c
      src/bacnet/basic/client/bac-discover.c
      src/bacnet/basic/client/bac-rw.c)
    target_link_libraries(bacdiscover PRIVATE ${PROJECT_NAME})
    target_compile_options(bacdiscover PRIVATE
      # Unreachable code because we have endless loop.
      $<$<C_COMPILER_ID:MSVC>:/wd4702>
    )
  endif(BACNET_BUILD_BACDISCOVER_APP)

  if(BACDL_BIP)
    add_executable(readbdt apps/readbdt/main.c)
    target_link_libraries(readbdt PRIVATE ${PROJECT_NAME})

    add_executable(readfdt apps/readfdt/main.c)
    target_link_libraries(readfdt PRIVATE ${PROJECT_NAME})
    endif()
  endif(BAC_ROUTING)

  add_executable(readfile apps/readfile/main.c)
  target_link_libraries(readfile PRIVATE ${PROJECT_NAME})

  add_executable(readprop apps/readprop/main.c)
  target_link_libraries(readprop PRIVATE ${PROJECT_NAME})

  add_executable(readpropm apps/readpropm/main.c)
  target_link_libraries(readpropm PRIVATE ${PROJECT_NAME})

  add_executable(readrange apps/readrange/main.c)
  target_link_libraries(readrange PRIVATE ${PROJECT_NAME})

  add_executable(remove-list-element apps/remove-list-element/main.c)
  target_link_libraries(remove-list-element PRIVATE ${PROJECT_NAME})

  add_executable(reinit apps/reinit/main.c)
  target_link_libraries(reinit PRIVATE ${PROJECT_NAME})

  if(BACDL_MSTP AND NOT WIN32)
    find_library(LIBCONFIG_LIBRARIES NAMES config)
    if(NOT LIBCONFIG_LIBRARIES)
      message(WARNING "BACNET: Will not build apps/router as libconfig not found")
      return()
    else()
      add_executable(
        router
        apps/router/ipmodule.c
        apps/router/ipmodule.h
        apps/router/main.c
        apps/router/msgqueue.c
        apps/router/msgqueue.h
        apps/router/mstpmodule.c
        apps/router/mstpmodule.h
        apps/router/network_layer.c
        apps/router/network_layer.h
        apps/router/portthread.c
        apps/router/portthread.h)

      target_link_libraries(
        router
        PRIVATE ${PROJECT_NAME}
                # needs libconfig
                -lconfig)

      target_compile_options(router PRIVATE
        # These make this example not totally C90 compatible but it is ok.

        -Wno-declaration-after-statement
        -Wno-overlength-strings
        -Wno-variadic-macros
      )
    endif()
  endif()

  if(BACDL_BIP AND BACDL_BIP6)
    add_executable(router-ipv6 apps/router-ipv6/main.c)
    target_link_libraries(router-ipv6 PRIVATE ${PROJECT_NAME})
  endif()

  add_executable(scov apps/scov/main.c)
  target_link_libraries(scov PRIVATE ${PROJECT_NAME})

  add_executable(server apps/server/main.c)
  target_link_libraries(server PRIVATE ${PROJECT_NAME})
  target_compile_options(server PRIVATE
    # Unreachable code because we have endless loop.
    $<$<C_COMPILER_ID:MSVC>:/wd4702>
  )

  add_executable(timesync apps/timesync/main.c)
  target_link_libraries(timesync PRIVATE ${PROJECT_NAME})

  add_executable(ucov apps/ucov/main.c)
  target_link_libraries(ucov PRIVATE ${PROJECT_NAME})

  add_executable(event apps/event/main.c)
  target_link_libraries(event PRIVATE ${PROJECT_NAME})

  add_executable(uevent apps/uevent/main.c)
  target_link_libraries(uevent PRIVATE ${PROJECT_NAME})

  add_executable(uptransfer apps/uptransfer/main.c)
  target_link_libraries(uptransfer PRIVATE ${PROJECT_NAME})

  add_executable(whohas apps/whohas/main.c)
  target_link_libraries(whohas PRIVATE ${PROJECT_NAME})

  add_executable(whois apps/whois/main.c)
  target_link_libraries(whois PRIVATE ${PROJECT_NAME})

  add_executable(whoisrouter apps/whoisrouter/main.c)
  target_link_libraries(whoisrouter PRIVATE ${PROJECT_NAME})

  add_executable(whatisnetnum apps/whatisnetnum/main.c)
  target_link_libraries(whatisnetnum PRIVATE ${PROJECT_NAME})

  add_executable(netnumis apps/netnumis/main.c)
  target_link_libraries(netnumis PRIVATE ${PROJECT_NAME})

  add_executable(writefile apps/writefile/main.c)
  target_link_libraries(writefile PRIVATE ${PROJECT_NAME})

  add_executable(writeprop apps/writeprop/main.c)
  target_link_libraries(writeprop PRIVATE ${PROJECT_NAME})

  add_executable(writepropm apps/writepropm/main.c)
  target_link_libraries(writepropm PRIVATE ${PROJECT_NAME})
endif()
endif(BACNET_STACK_BUILD_APPS)

#
# install
#

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(BACNET_STACK_CONFIG_INSTALL_DIR ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME})
install(
  TARGETS ${PROJECT_NAME}
  EXPORT ${PROJECT_NAME}Targets
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT lib
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT lib)

install(
  EXPORT ${PROJECT_NAME}Targets
  DESTINATION ${BACNET_STACK_CONFIG_INSTALL_DIR}
  NAMESPACE ${PROJECT_NAME}::
  COMPONENT dev)

configure_package_config_file(
  cmake/Config.cmake.in
  ${PROJECT_NAME}Config.cmake
  INSTALL_DESTINATION
  ${BACNET_STACK_CONFIG_INSTALL_DIR})
write_basic_package_version_file(${PROJECT_NAME}ConfigVersion.cmake COMPATIBILITY SameMajorVersion)
install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
  DESTINATION ${BACNET_STACK_CONFIG_INSTALL_DIR}
  COMPONENT dev)

install(
  DIRECTORY src/bacnet
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  COMPONENT dev
  FILES_MATCHING
  PATTERN "*.h")

install(
  DIRECTORY ${BACNET_PORT_DIRECTORY_PATH}/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  COMPONENT dev
  FILES_MATCHING
  PATTERN "*.h")

message(STATUS "BACNET: using cmake:....................\"${CMAKE_VERSION}\"")
message(STATUS "BACNET: CMAKE_C_COMPILER_ID:............\"${CMAKE_C_COMPILER_ID}\"")
message(STATUS "BACNET: CMAKE_C_COMPILER_VERSION:.......\"${CMAKE_C_COMPILER_VERSION}\"")
message(STATUS "BACNET: CMAKE_CXX_COMPILER_ID:..........\"${CMAKE_CXX_COMPILER_ID}\"")
message(STATUS "BACNET: CMAKE_CXX_COMPILER_VERSION:.....\"${CMAKE_CXX_COMPILER_VERSION}\"")
message(STATUS "BACNET: CMAKE_BUILD_TYPE:...............\"${CMAKE_BUILD_TYPE}\"")
message(STATUS "BACNET: CMAKE_INSTALL_PREFIX:...........\"${CMAKE_INSTALL_PREFIX}\"")
message(STATUS "BACNET: BACNET_PROTOCOL_REVISION:.......\"${BACNET_PROTOCOL_REVISION}\"")
message(STATUS "BACNET: Selected datalinks:")
message(STATUS "BACNET: BACDL_BIP6:.....................\"${BACDL_BIP6}\"")
message(STATUS "BACNET: BACDL_BIP:......................\"${BACDL_BIP}\"")
message(STATUS "BACNET: BACDL_ARCNET:...................\"${BACDL_ARCNET}\"")
message(STATUS "BACNET: BACDL_MSTP:.....................\"${BACDL_MSTP}\"")
message(STATUS "BACNET: BACDL_ETHERNET:.................\"${BACDL_ETHERNET}\"")
message(STATUS "BACNET: BACDL_NONE:.....................\"${BACDL_NONE}\"")
message(STATUS "BACNET: BAC_ROUTING:....................\"${BAC_ROUTING}\"")
