cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

project(
  bacnet-stack
  VERSION 1.1.0
  LANGUAGES C)

#
# options
#

option(
  BACNET_STACK_BUILD_APPS
  "build apps"
  ON)

option(
  BAC_ROUTING
  "enable bac routing"
  ON)

option(
  BACNET_PROPERTY_LISTS
  "enable property lists"
  ON)

option(
  BACNET_BUILD_PIFACE_APP
  "compile the piface app"
  OFF)

option(
  BACNET_BUILD_BACPOLL_APP
  "compile the bacpoll app"
  ON)

option(
  BACDL_ETHERNET
  "compile with ethernet support"
  OFF)

option(
  BACDL_MSTP
  "compile with mstp support"
  OFF)

option(
  BACDL_ARCNET
  "compile with arcnet support"
  OFF)

option(
  BACDL_BIP
  "compile with ip support"
  ON)

option(
  BACDL_BIP6
  "compile with ipv6 support"
  OFF)

option(
  BACDL_NONE
  "compile without datalink"
  OFF)

set(BACNET_PROTOCOL_REVISION 19)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

#
# library
#

find_package(Threads)

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
    src/bacnet/bacapp.c
    src/bacnet/bacapp.h
    src/bacnet/bacdcode.c
    src/bacnet/bacdcode.h
    src/bacnet/bacdef.h
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
    src/bacnet/basic/npdu/h_routed_npdu.c
    src/bacnet/basic/npdu/h_routed_npdu.h
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
    src/bacnet/basic/object/bo.c
    src/bacnet/basic/object/bo.h
    src/bacnet/basic/object/bv.c
    src/bacnet/basic/object/bv.h
    src/bacnet/basic/object/channel.c
    src/bacnet/basic/object/channel.h
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
    src/bacnet/basic/service/h_cov.c
    src/bacnet/basic/service/h_cov.h
    src/bacnet/basic/service/h_dcc.c
    src/bacnet/basic/service/h_dcc.h
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
    src/bacnet/basic/service/s_cov.c
    src/bacnet/basic/service/s_cov.h
    src/bacnet/basic/service/s_dcc.c
    src/bacnet/basic/service/s_dcc.h
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
    src/bacnet/basic/sys/mstimer.c
    src/bacnet/basic/sys/mstimer.h
    src/bacnet/basic/sys/ringbuf.c
    src/bacnet/basic/sys/ringbuf.h
    src/bacnet/basic/sys/sbuf.c
    src/bacnet/basic/sys/sbuf.h
    src/bacnet/basic/tsm/tsm.c
    src/bacnet/basic/tsm/tsm.h
    src/bacnet/bits.h
    src/bacnet/bytes.h
    src/bacnet/config.h
    src/bacnet/cov.c
    src/bacnet/cov.h
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
    src/bacnet/wpm.h)
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
target_compile_definitions(
  ${PROJECT_NAME}
  PUBLIC
  BACNET_PROTOCOL_REVISION=${BACNET_PROTOCOL_REVISION}
  $<$<BOOL:${BACDL_BIP}>:BACDL_BIP>
  $<$<BOOL:${BACDL_BIP6}>:BACDL_BIP6>
  $<$<BOOL:${BACDL_ARCNET}>:BACDL_ARCNET>
  $<$<BOOL:${BACDL_MSTP}>:BACDL_MSTP>
  $<$<BOOL:${BACDL_ETHERNET}>:BACDL_ETHERNET>
  $<$<BOOL:${BACDL_NONE}>:BACDL_NONE>
  $<$<BOOL:${BACNET_PROPERTY_LISTS}>:BACNET_PROPERTY_LISTS>
  $<$<BOOL:${BAC_ROUTING}>:BAC_ROUTING>
  $<$<NOT:$<BOOL:${BUILD_SHARED_LIBS}>>:BACNET_STACK_STATIC_DEFINE>
  PRIVATE
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

  target_sources(${PROJECT_NAME} PRIVATE
    ports/linux/bacport.h
    ports/linux/datetime-init.c
    $<$<BOOL:${BACDL_BIP}>:ports/linux/bip-init.c>
    $<$<BOOL:${BACDL_BIP6}>:ports/linux/bip6.c>
    $<$<BOOL:${BACDL_ARCNET}>:ports/linux/arcnet.c>
    $<$<BOOL:${BACDL_MSTP}>:ports/linux/rs485.c>
    $<$<BOOL:${BACDL_MSTP}>:ports/linux/rs485.h>
    $<$<BOOL:${BACDL_MSTP}>:ports/linux/dlmstp.c>
    $<$<BOOL:${BACDL_MSTP}>:ports/linux/dlmstp_linux.c>
    $<$<BOOL:${BACDL_MSTP}>:ports/linux/dlmstp_linux.h>
    # ports/linux/rx_fsm.c
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
    # ports/win32/rx_fsm.c
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

  target_sources(${PROJECT_NAME} PRIVATE
    ports/bsd/bacport.h
    ports/bsd/bip-init.c
    ports/bsd/datetime-init.c
    ports/bsd/mstimer-init.c
    ports/bsd/stdbool.h)
endif()

target_include_directories(${PROJECT_NAME} PUBLIC
  $<BUILD_INTERFACE:${BACNET_PORT_DIRECTORY_PATH}>)

#
# examples
#

if(BACNET_STACK_BUILD_APPS)
  message(STATUS "BACNET: compiling also apps")

  add_executable(abort apps/abort/main.c)
  target_link_libraries(abort PRIVATE ${PROJECT_NAME})

  add_executable(ack-alarm apps/ack-alarm/main.c)
  target_link_libraries(ack-alarm PRIVATE ${PROJECT_NAME})

  add_executable(dcc apps/dcc/main.c)
  target_link_libraries(dcc PRIVATE ${PROJECT_NAME})

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

    add_executable(mstpcrc apps/mstpcrc/main.c)
    target_link_libraries(mstpcrc PRIVATE ${PROJECT_NAME})
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
  endif(BACNET_BUILD_BACPOLL_APP)

  if(NOT BACDL_ETHERNET)
    add_executable(readbdt apps/readbdt/main.c)
    target_link_libraries(readbdt PRIVATE ${PROJECT_NAME})

    add_executable(readfdt apps/readfdt/main.c)
    target_link_libraries(readfdt PRIVATE ${PROJECT_NAME})
  endif()

  add_executable(readfile apps/readfile/main.c)
  target_link_libraries(readfile PRIVATE ${PROJECT_NAME})

  add_executable(readprop apps/readprop/main.c)
  target_link_libraries(readprop PRIVATE ${PROJECT_NAME})

  add_executable(readpropm apps/readpropm/main.c)
  target_link_libraries(readpropm PRIVATE ${PROJECT_NAME})

  add_executable(readrange apps/readrange/main.c)
  target_link_libraries(readrange PRIVATE ${PROJECT_NAME})

  add_executable(reinit apps/reinit/main.c)
  target_link_libraries(reinit PRIVATE ${PROJECT_NAME})

  if(BACDL_MSTP)
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
  endif()

  if(BACDL_BIP6)
    add_executable(router-ipv6 apps/router-ipv6/main.c)
    target_link_libraries(router-ipv6 PRIVATE ${PROJECT_NAME})
  endif()

  add_executable(scov apps/scov/main.c)
  target_link_libraries(scov PRIVATE ${PROJECT_NAME})

  add_executable(server apps/server/main.c)
  target_link_libraries(server PRIVATE ${PROJECT_NAME})

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

#
# install
#

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(BACNET_STACK_CONFIG_INSTALL_DIR ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME})
install(
  TARGETS ${PROJECT_NAME}
  EXPORT ${PROJECT_NAME}Targets
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
message(STATUS "BACNET: BACDL_BIP6:.....................\"${BACDL_BIP6}\"")
message(STATUS "BACNET: BACDL_BIP:......................\"${BACDL_BIP}\"")
message(STATUS "BACNET: BACDL_ARCNET:...................\"${BACDL_ARCNET}\"")
message(STATUS "BACNET: BACDL_MSTP:.....................\"${BACDL_MSTP}\"")
message(STATUS "BACNET: BACDL_ETHERNET:.................\"${BACDL_ETHERNET}\"")
message(STATUS "BACNET: BACDL_NONE:.....................\"${BACDL_NONE}\"")
