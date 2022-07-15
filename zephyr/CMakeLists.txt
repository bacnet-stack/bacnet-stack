cmake_minimum_required(VERSION 3.10 FATAL_ERROR)

# Detect the platform reliably
if(ZEPHYR_BASE)
  if (NOT CONFIG_BACNETSTACK)
    return()
  endif()
  set(ZEPHYR YES)
else()
  message(FATAL_ERROR "ZEPHYR_BASE needs to be defined for Zephyr builds")
endif()

#
# options managed through Kconfig and use names CONFIG_*
#

set(BACNET_PROTOCOL_REVISION 19)

message(STATUS "BACNETSTACK: using cmake ${CMAKE_VERSION}")
message(STATUS "BACNETSTACK: CMAKE_C_COMPILER_ID \"${CMAKE_C_COMPILER_ID}\"")
message(STATUS "BACNETSTACK: CMAKE_C_COMPILER_VERSION \"${CMAKE_C_COMPILER_VERSION}\"")
message(STATUS "BACNETSTACK: CMAKE_CXX_COMPILER_ID \"${CMAKE_CXX_COMPILER_ID}\"")
message(STATUS "BACNETSTACK: CMAKE_CXX_COMPILER_VERSION \"${CMAKE_CXX_COMPILER_VERSION}\"")
message(STATUS "BACNETSTACK: CMAKE_INSTALL_PREFIX \"${CMAKE_INSTALL_PREFIX}\"")
message(STATUS "BACNETSTACK: BACNET_PROTOCOL_REVISION \"${BACNET_PROTOCOL_REVISION}\"")
message(STATUS "BACNETSTACK: BACDL_BIP6 \"${CONFIG_BACDL_BIP6}\"")
message(STATUS "BACNETSTACK: BACDL_BIP \"${CONFIG_BACDL_BIP}\"")
message(STATUS "BACNETSTACK: BACDL_ARCNET \"${CONFIG_BACDL_ARCNET}\"")
message(STATUS "BACNETSTACK: BACDL_MSTP \"${CONFIG_BACDL_MSTP}\"")
message(STATUS "BACNETSTACK: BACDL_ETHERNET \"${CONFIG_BACDL_ETHERNET}\"")
message(STATUS "BACNETSTACK: BACDL_NONE \"${CONFIG_BACDL_NONE}\"")
message(STATUS "BACNETSTACK: BACNET_ADDRESS_CACHE_FILE \"${CONFIG_BACNET_ADDRESS_CACHE_FILE}\"")

#Do not allow in source builds
set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)

#
# sources
#
set(BACNETSTACK_SRC ../src)

set(BACNETSTACK_SRCS
    ${BACNETSTACK_SRC}/bacnet/abort.c
    ${BACNETSTACK_SRC}/bacnet/abort.h
    ${BACNETSTACK_SRC}/bacnet/access_rule.c
    ${BACNETSTACK_SRC}/bacnet/access_rule.h
    ${BACNETSTACK_SRC}/bacnet/alarm_ack.c
    ${BACNETSTACK_SRC}/bacnet/alarm_ack.h
    ${BACNETSTACK_SRC}/bacnet/apdu.h
    ${BACNETSTACK_SRC}/bacnet/arf.c
    ${BACNETSTACK_SRC}/bacnet/arf.h
    ${BACNETSTACK_SRC}/bacnet/assigned_access_rights.c
    ${BACNETSTACK_SRC}/bacnet/assigned_access_rights.h
    ${BACNETSTACK_SRC}/bacnet/authentication_factor.c
    ${BACNETSTACK_SRC}/bacnet/authentication_factor_format.c
    ${BACNETSTACK_SRC}/bacnet/authentication_factor_format.h
    ${BACNETSTACK_SRC}/bacnet/authentication_factor.h
    ${BACNETSTACK_SRC}/bacnet/awf.c
    ${BACNETSTACK_SRC}/bacnet/awf.h
    ${BACNETSTACK_SRC}/bacnet/bacaddr.c
    ${BACNETSTACK_SRC}/bacnet/bacaddr.h
    ${BACNETSTACK_SRC}/bacnet/bacapp.c
    ${BACNETSTACK_SRC}/bacnet/bacapp.h
    ${BACNETSTACK_SRC}/bacnet/bacdcode.c
    ${BACNETSTACK_SRC}/bacnet/bacdcode.h
    ${BACNETSTACK_SRC}/bacnet/bacdef.h
    ${BACNETSTACK_SRC}/bacnet/bacdevobjpropref.c
    ${BACNETSTACK_SRC}/bacnet/bacdevobjpropref.h
    ${BACNETSTACK_SRC}/bacnet/bacenum.h
    ${BACNETSTACK_SRC}/bacnet/bacerror.c
    ${BACNETSTACK_SRC}/bacnet/bacerror.h
    ${BACNETSTACK_SRC}/bacnet/bacint.c
    ${BACNETSTACK_SRC}/bacnet/bacint.h
    ${BACNETSTACK_SRC}/bacnet/bacprop.c
    ${BACNETSTACK_SRC}/bacnet/bacprop.h
    ${BACNETSTACK_SRC}/bacnet/bacpropstates.c
    ${BACNETSTACK_SRC}/bacnet/bacpropstates.h
    ${BACNETSTACK_SRC}/bacnet/bacreal.c
    ${BACNETSTACK_SRC}/bacnet/bacreal.h
    ${BACNETSTACK_SRC}/bacnet/bacstr.c
    ${BACNETSTACK_SRC}/bacnet/bacstr.h
    ${BACNETSTACK_SRC}/bacnet/bactext.c
    ${BACNETSTACK_SRC}/bacnet/bactext.h
    ${BACNETSTACK_SRC}/bacnet/bactimevalue.c
    ${BACNETSTACK_SRC}/bacnet/bactimevalue.h
    $<$<BOOL:${CONFIG_BACDL_BIP}>:${BACNETSTACK_SRC}/bacnet/basic/bbmd/h_bbmd.c>
    $<$<BOOL:${CONFIG_BACDL_BIP}>:${BACNETSTACK_SRC}/bacnet/basic/bbmd/h_bbmd.h>
    $<$<BOOL:${CONFIG_BACDL_BIP6}>:${BACNETSTACK_SRC}/bacnet/basic/bbmd6/h_bbmd6.c>
    $<$<BOOL:${CONFIG_BACDL_BIP6}>:${BACNETSTACK_SRC}/bacnet/basic/bbmd6/h_bbmd6.h>
    $<$<BOOL:${CONFIG_BACDL_BIP6}>:${BACNETSTACK_SRC}/bacnet/basic/bbmd6/vmac.c>
    $<$<BOOL:${CONFIG_BACDL_BIP6}>:${BACNETSTACK_SRC}/bacnet/basic/bbmd6/vmac.h>
    ${BACNETSTACK_SRC}/bacnet/basic/binding/address.c
    ${BACNETSTACK_SRC}/bacnet/basic/binding/address.h
    ${BACNETSTACK_SRC}/bacnet/basic/npdu/h_npdu.c
    ${BACNETSTACK_SRC}/bacnet/basic/npdu/h_npdu.h
    ${BACNETSTACK_SRC}/bacnet/basic/npdu/h_routed_npdu.c
    ${BACNETSTACK_SRC}/bacnet/basic/npdu/h_routed_npdu.h
    ${BACNETSTACK_SRC}/bacnet/basic/npdu/s_router.h
    ${BACNETSTACK_SRC}/bacnet/basic/object/access_credential.h
    ${BACNETSTACK_SRC}/bacnet/basic/object/access_door.h
    ${BACNETSTACK_SRC}/bacnet/basic/object/access_point.h
    ${BACNETSTACK_SRC}/bacnet/basic/object/access_rights.h
    ${BACNETSTACK_SRC}/bacnet/basic/object/access_user.h
    ${BACNETSTACK_SRC}/bacnet/basic/object/access_zone.h
    ${BACNETSTACK_SRC}/bacnet/basic/object/ai.h
    ${BACNETSTACK_SRC}/bacnet/basic/object/ao.h
    ${BACNETSTACK_SRC}/bacnet/basic/object/av.h
    ${BACNETSTACK_SRC}/bacnet/basic/object/bacfile.h
    ${BACNETSTACK_SRC}/bacnet/basic/object/bi.h
    ${BACNETSTACK_SRC}/bacnet/basic/object/bo.h
    ${BACNETSTACK_SRC}/bacnet/basic/object/bv.h
    ${BACNETSTACK_SRC}/bacnet/basic/object/channel.h
    ${BACNETSTACK_SRC}/bacnet/basic/object/command.h
    ${BACNETSTACK_SRC}/bacnet/basic/object/credential_data_input.h
    ${BACNETSTACK_SRC}/bacnet/basic/object/csv.h
    ${BACNETSTACK_SRC}/bacnet/basic/object/device.h
    ${BACNETSTACK_SRC}/bacnet/basic/object/iv.h
    ${BACNETSTACK_SRC}/bacnet/basic/object/lc.h
    ${BACNETSTACK_SRC}/bacnet/basic/object/lo.h
    ${BACNETSTACK_SRC}/bacnet/basic/object/lsp.h
    ${BACNETSTACK_SRC}/bacnet/basic/object/ms-input.h
    ${BACNETSTACK_SRC}/bacnet/basic/object/mso.h
    ${BACNETSTACK_SRC}/bacnet/basic/object/msv.h
    ${BACNETSTACK_SRC}/bacnet/basic/object/nc.h
    ${BACNETSTACK_SRC}/bacnet/basic/object/netport.h
    ${BACNETSTACK_SRC}/bacnet/basic/object/objects.h
    ${BACNETSTACK_SRC}/bacnet/basic/object/osv.h
    ${BACNETSTACK_SRC}/bacnet/basic/object/piv.h
    ${BACNETSTACK_SRC}/bacnet/basic/object/schedule.h
    ${BACNETSTACK_SRC}/bacnet/basic/object/trendlog.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_alarm_ack.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_apdu.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_apdu.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_arf_a.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_arf.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_awf.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_ccov.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_cov.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_cov.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_dcc.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_dcc.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_gas_a.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_get_alarm_sum.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_getevent_a.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_getevent.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_iam.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_iam.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_ihave.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_ihave.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_lso.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_noserv.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_noserv.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_rd.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_rd.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_rp_a.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_rp_a.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_rp.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_rp.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_rpm_a.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_rpm_a.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_rpm.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_rpm.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_rr_a.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_rr.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_ts.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_ucov.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_upt.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_whohas.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_whohas.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_whois.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_whois.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_wp.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_wp.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_wpm.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_wpm.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_abort.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_ack_alarm.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_arfs.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_awfs.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_cevent.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_cov.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_dcc.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_error.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_get_alarm_sum.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_get_event.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_getevent.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_iam.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_iam.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_ihave.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_ihave.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_lso.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_rd.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_readrange.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_rp.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_rpm.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_ts.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_uevent.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_upt.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_whohas.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_whohas.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_whois.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_whois.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_wp.h
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_wpm.h
    ${BACNETSTACK_SRC}/bacnet/basic/services.h
    ${BACNETSTACK_SRC}/bacnet/basic/sys/bigend.c
    ${BACNETSTACK_SRC}/bacnet/basic/sys/bigend.h
    ${BACNETSTACK_SRC}/bacnet/basic/sys/days.c
    ${BACNETSTACK_SRC}/bacnet/basic/sys/days.h
    ${BACNETSTACK_SRC}/bacnet/basic/sys/debug.c
    ${BACNETSTACK_SRC}/bacnet/basic/sys/debug.h
    ${BACNETSTACK_SRC}/bacnet/basic/sys/fifo.c
    ${BACNETSTACK_SRC}/bacnet/basic/sys/fifo.h
    ${BACNETSTACK_SRC}/bacnet/basic/sys/filename.c
    ${BACNETSTACK_SRC}/bacnet/basic/sys/filename.h
    ${BACNETSTACK_SRC}/bacnet/basic/sys/key.h
    ${BACNETSTACK_SRC}/bacnet/basic/sys/keylist.c
    ${BACNETSTACK_SRC}/bacnet/basic/sys/keylist.h
    ${BACNETSTACK_SRC}/bacnet/basic/sys/mstimer.c
    ${BACNETSTACK_SRC}/bacnet/basic/sys/mstimer.h
    ${BACNETSTACK_SRC}/bacnet/basic/sys/ringbuf.c
    ${BACNETSTACK_SRC}/bacnet/basic/sys/ringbuf.h
    ${BACNETSTACK_SRC}/bacnet/basic/sys/sbuf.c
    ${BACNETSTACK_SRC}/bacnet/basic/sys/sbuf.h
    ${BACNETSTACK_SRC}/bacnet/basic/tsm/tsm.c
    ${BACNETSTACK_SRC}/bacnet/basic/tsm/tsm.h
    ${BACNETSTACK_SRC}/bacnet/bits.h
    ${BACNETSTACK_SRC}/bacnet/bytes.h
    ${BACNETSTACK_SRC}/bacnet/config.h
    ${BACNETSTACK_SRC}/bacnet/cov.c
    ${BACNETSTACK_SRC}/bacnet/cov.h
    ${BACNETSTACK_SRC}/bacnet/credential_authentication_factor.c
    ${BACNETSTACK_SRC}/bacnet/credential_authentication_factor.h
    ${BACNETSTACK_SRC}/bacnet/datalink/arcnet.h
    ${BACNETSTACK_SRC}/bacnet/datalink/bacsec.c
    ${BACNETSTACK_SRC}/bacnet/datalink/bacsec.h
    ${BACNETSTACK_SRC}/bacnet/datalink/bip6.h
    $<$<BOOL:${CONFIG_BACDL_BIP}>:${BACNETSTACK_SRC}/bacnet/datalink/bip.h>
    $<$<BOOL:${CONFIG_BACDL_BIP6}>:${BACNETSTACK_SRC}/bacnet/datalink/bvlc6.c>
    $<$<BOOL:${CONFIG_BACDL_BIP6}>:${BACNETSTACK_SRC}/bacnet/datalink/bvlc6.h>
    $<$<BOOL:${CONFIG_BACDL_BIP}>:${BACNETSTACK_SRC}/bacnet/datalink/bvlc.h>
    $<$<BOOL:${CONFIG_BACDL_BIP}>:${BACNETSTACK_SRC}/bacnet/datalink/bvlc.c>
    $<$<BOOL:${CONFIG_BACDL_MSTP}>:${BACNETSTACK_SRC}/bacnet/datalink/crc.h>
    $<$<BOOL:${CONFIG_BACDL_MSTP}>:${BACNETSTACK_SRC}/bacnet/datalink/crc.c>
    ${BACNETSTACK_SRC}/bacnet/datalink/datalink.c
    ${BACNETSTACK_SRC}/bacnet/datalink/datalink.h
    ${BACNETSTACK_SRC}/bacnet/datalink/dlmstp.h
    ${BACNETSTACK_SRC}/bacnet/datalink/ethernet.h
    $<$<BOOL:${CONFIG_BACDL_MSTP}>:${BACNETSTACK_SRC}/bacnet/datalink/mstp.h>
    ${BACNETSTACK_SRC}/bacnet/datalink/mstpdef.h
    ${BACNETSTACK_SRC}/bacnet/datalink/mstp.h
    ${BACNETSTACK_SRC}/bacnet/datalink/mstptext.c
    ${BACNETSTACK_SRC}/bacnet/datalink/mstptext.h
    ${BACNETSTACK_SRC}/bacnet/datetime.c
    ${BACNETSTACK_SRC}/bacnet/datetime.h
    ${BACNETSTACK_SRC}/bacnet/dcc.c
    ${BACNETSTACK_SRC}/bacnet/dcc.h
    ${BACNETSTACK_SRC}/bacnet/event.h
    ${BACNETSTACK_SRC}/bacnet/get_alarm_sum.c
    ${BACNETSTACK_SRC}/bacnet/get_alarm_sum.h
    ${BACNETSTACK_SRC}/bacnet/getevent.c
    ${BACNETSTACK_SRC}/bacnet/getevent.h
    ${BACNETSTACK_SRC}/bacnet/hostnport.c
    ${BACNETSTACK_SRC}/bacnet/hostnport.h
    ${BACNETSTACK_SRC}/bacnet/iam.c
    ${BACNETSTACK_SRC}/bacnet/iam.h
    ${BACNETSTACK_SRC}/bacnet/ihave.c
    ${BACNETSTACK_SRC}/bacnet/ihave.h
    ${BACNETSTACK_SRC}/bacnet/indtext.c
    ${BACNETSTACK_SRC}/bacnet/indtext.h
    ${BACNETSTACK_SRC}/bacnet/lighting.c
    ${BACNETSTACK_SRC}/bacnet/lighting.h
    ${BACNETSTACK_SRC}/bacnet/lso.c
    ${BACNETSTACK_SRC}/bacnet/lso.h
    ${BACNETSTACK_SRC}/bacnet/memcopy.c
    ${BACNETSTACK_SRC}/bacnet/memcopy.h
    ${BACNETSTACK_SRC}/bacnet/npdu.c
    ${BACNETSTACK_SRC}/bacnet/npdu.h
    ${BACNETSTACK_SRC}/bacnet/property.c
    ${BACNETSTACK_SRC}/bacnet/property.h
    ${BACNETSTACK_SRC}/bacnet/proplist.c
    ${BACNETSTACK_SRC}/bacnet/proplist.h
    ${BACNETSTACK_SRC}/bacnet/ptransfer.c
    ${BACNETSTACK_SRC}/bacnet/ptransfer.h
    ${BACNETSTACK_SRC}/bacnet/rd.c
    ${BACNETSTACK_SRC}/bacnet/rd.h
    ${BACNETSTACK_SRC}/bacnet/readrange.c
    ${BACNETSTACK_SRC}/bacnet/readrange.h
    ${BACNETSTACK_SRC}/bacnet/reject.c
    ${BACNETSTACK_SRC}/bacnet/reject.h
    ${BACNETSTACK_SRC}/bacnet/rp.c
    ${BACNETSTACK_SRC}/bacnet/rp.h
    ${BACNETSTACK_SRC}/bacnet/rpm.c
    ${BACNETSTACK_SRC}/bacnet/rpm.h
    ${BACNETSTACK_SRC}/bacnet/timestamp.c
    ${BACNETSTACK_SRC}/bacnet/timestamp.h
    ${BACNETSTACK_SRC}/bacnet/timesync.c
    ${BACNETSTACK_SRC}/bacnet/timesync.h
    ${BACNETSTACK_SRC}/bacnet/version.h
    ${BACNETSTACK_SRC}/bacnet/whohas.c
    ${BACNETSTACK_SRC}/bacnet/whohas.h
    ${BACNETSTACK_SRC}/bacnet/whois.c
    ${BACNETSTACK_SRC}/bacnet/whois.h
    ${BACNETSTACK_SRC}/bacnet/wp.c
    ${BACNETSTACK_SRC}/bacnet/wp.h
    ${BACNETSTACK_SRC}/bacnet/wpm.c
    ${BACNETSTACK_SRC}/bacnet/wpm.h
    )

set(BACNETSTACK_BASIC_SRCS
    $<$<BOOL:${CONFIG_BACDL_BIP6}>:${BACNETSTACK_SRC}/bacnet/basic/bbmd6/h_bbmd6.c>
    $<$<BOOL:${CONFIG_BACDL_BIP6}>:${BACNETSTACK_SRC}/bacnet/basic/bbmd6/vmac.c>
    ${BACNETSTACK_SRC}/bacnet/basic/npdu/s_router.c
    ${BACNETSTACK_SRC}/bacnet/basic/object/access_credential.c
    ${BACNETSTACK_SRC}/bacnet/basic/object/access_door.c
    ${BACNETSTACK_SRC}/bacnet/basic/object/access_point.c
    ${BACNETSTACK_SRC}/bacnet/basic/object/access_rights.c
    ${BACNETSTACK_SRC}/bacnet/basic/object/access_user.c
    ${BACNETSTACK_SRC}/bacnet/basic/object/access_zone.c
    ${BACNETSTACK_SRC}/bacnet/basic/object/acc.c
    ${BACNETSTACK_SRC}/bacnet/basic/object/ao.c
    ${BACNETSTACK_SRC}/bacnet/basic/object/bacfile.c
    ${BACNETSTACK_SRC}/bacnet/basic/object/bi.c
    ${BACNETSTACK_SRC}/bacnet/basic/object/bo.c
    ${BACNETSTACK_SRC}/bacnet/basic/object/bv.c
    ${BACNETSTACK_SRC}/bacnet/basic/object/channel.c
    #${BACNETSTACK_SRC}/bacnet/basic/object/client/device-client.c
    ${BACNETSTACK_SRC}/bacnet/basic/object/command.c
    ${BACNETSTACK_SRC}/bacnet/basic/object/credential_data_input.c
    ${BACNETSTACK_SRC}/bacnet/basic/object/csv.c
    ${BACNETSTACK_SRC}/bacnet/basic/object/device.c
    $<$<BOOL:${CONFIG_BAC_ROUTING}>:${BACNETSTACK_SRC}/bacnet/basic/object/gateway/gw_device.c>
    ${BACNETSTACK_SRC}/bacnet/basic/object/iv.c
    ${BACNETSTACK_SRC}/bacnet/basic/object/lc.c
    ${BACNETSTACK_SRC}/bacnet/basic/object/lo.c
    ${BACNETSTACK_SRC}/bacnet/basic/object/lsp.c
    ${BACNETSTACK_SRC}/bacnet/basic/object/ms-input.c
    ${BACNETSTACK_SRC}/bacnet/basic/object/mso.c
    ${BACNETSTACK_SRC}/bacnet/basic/object/msv.c
    ${BACNETSTACK_SRC}/bacnet/basic/object/nc.c
    $<$<BOOL:${CONFIG_BACDL_BIP}>:${BACNETSTACK_SRC}/bacnet/basic/object/netport.c>
    ${BACNETSTACK_SRC}/bacnet/basic/object/objects.c
    ${BACNETSTACK_SRC}/bacnet/basic/object/osv.c
    ${BACNETSTACK_SRC}/bacnet/basic/object/piv.c
    ${BACNETSTACK_SRC}/bacnet/basic/object/schedule.c
    ${BACNETSTACK_SRC}/bacnet/basic/object/trendlog.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_alarm_ack.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_arf_a.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_arf.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_awf.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_ccov.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_gas_a.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_get_alarm_sum.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_getevent_a.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_getevent.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_lso.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_rr_a.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_rr.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_ts.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_ucov.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/h_upt.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_abort.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_ack_alarm.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_arfs.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_awfs.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_cevent.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_cov.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_dcc.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_error.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_get_alarm_sum.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_get_event.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_getevent.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_lso.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_rd.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_readrange.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_rp.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_rpm.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_ts.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_uevent.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_upt.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_wp.c
    ${BACNETSTACK_SRC}/bacnet/basic/service/s_wpm.c
    )

#
# add ports
#

set(BACNETSTACK_PORT_DIRECTORY_PATH "")

message(STATUS "BACNETSTACK: building for Zephyr")
set(BACNETSTACK_PORT ${CMAKE_CURRENT_LIST_DIR}/../ports/zephyr)

list(
  APPEND BACNETSTACK_SRCS
      ${BACNETSTACK_BASIC_SRCS}
      ${BACNETSTACK_PORT}/datetime-init.c
      $<$<BOOL:${CONFIG_BACDL_BIP}>:${BACNETSTACK_PORT}/bip-init.c>
      $<$<BOOL:${CONFIG_BACDL_BIP6}>:${BACNETSTACK_PORT}/bip6-init.c>
      $<$<BOOL:${CONFIG_BACDL_BIP}>:${BACNETSTACK_PORT}/datalink/bvlc.c>
      $<$<BOOL:${CONFIG_BACDL_ETHERNET}>:${BACNETSTACK_PORT}/ethernet.c>
      ${BACNETSTACK_PORT}/main.c
      ${BACNETSTACK_PORT}/mstimer-init.c
      ${BACNETSTACK_PORT}/ai.c
      ${BACNETSTACK_PORT}/av.c
      ${BACNETSTACK_PORT}/event.c
  )


add_subdirectory(subsys)


#
# library
#

zephyr_library()

zephyr_library_compile_definitions(
  $<$<BOOL:${CONFIG_BACDL_BIP}>:BACDL_BIP>
  $<$<BOOL:${CONFIG_BACDL_BIP6}>:BACDL_BIP6>
  $<$<BOOL:${CONFIG_BACDL_ARCNET}>:BACDL_ARCNET>
  $<$<BOOL:${CONFIG_BACDL_MSTP}>:BACDL_MSTP>
  $<$<BOOL:${CONFIG_BACDL_ETHERNET}>:BACDL_ETHERNET>
  $<$<BOOL:${CONFIG_BACDL_NONE}>:BACDL_NONE>
  $<$<BOOL:${CONFIG_BACNET_PROPERTY_LISTS}>:BACNET_PROPERTY_LISTS=1>
  $<$<BOOL:${CONFIG_BACNET_ROUTING}>:BACNET_ROUTING>
  $<$<BOOL:${CONFIG_BACAPP_PRINT_ENABLED}>:BACAPP_PRINT_ENABLED=1>
  $<$<BOOL:${CONFIG_BACAPP_SNPRINTF_ENABLED}>:BACAPP_SNPRINTF_ENABLED=1>
  $<$<BOOL:${CONFIG_BACNET_ADDRESS_CACHE_FILE}>:BACNET_ADDRESS_CACHE_FILE=1>
  )

zephyr_library_sources(
  ${BACNETSTACK_SRCS}
  )

zephyr_include_directories(
  ${BACNETSTACK_PORT}
  ${BACNETSTACK_SRC}
  )

target_compile_definitions(
  app PRIVATE
  BACNET_PROTOCOL_REVISION=${BACNET_PROTOCOL_REVISION}
  $<$<BOOL:${CONFIG_BACDL_BIP}>:BACDL_BIP>
  $<$<BOOL:${CONFIG_BACDL_BIP6}>:BACDL_BIP6>
  $<$<BOOL:${CONFIG_BACDL_ARCNET}>:BACDL_ARCNET>
  $<$<BOOL:${CONFIG_BACDL_MSTP}>:BACDL_MSTP>
  $<$<BOOL:${CONFIG_BACDL_ETHERNET}>:BACDL_ETHERNET>
  $<$<BOOL:${CONFIG_BACDL_NONE}>:BACDL_NONE>
  $<$<BOOL:${CONFIG_BACNET_PROPERTY_LISTS}>:BACNET_PROPERTY_LISTS=1>
  $<$<BOOL:${CONFIG_BACNET_ROUTING}>:BACNET_ROUTING>
  $<$<BOOL:${CONFIG_BACAPP_PRINT_ENABLED}>:BACAPP_PRINT_ENABLED=1>
  $<$<BOOL:${CONFIG_BACAPP_SNPRINTF_ENABLED}>:BACAPP_SNPRINTF_ENABLED=1>
  $<$<BOOL:${CONFIG_BACNET_ADDRESS_CACHE_FILE}>:BACNET_ADDRESS_CACHE_FILE=1>
  BACNET_STACK_STATIC_DEFINE
  PRINT_ENABLED=1
  )

