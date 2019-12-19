cmake_minimum_required(VERSION 3.12 FATAL_ERROR)

project(bacnet-stack LANGUAGES C)

option(BACNET_STACK_BUILD_APPS
       "build apps"
       OFF)

option(BAC_ROUTING
       "enable bac routing"
       OFF)

set(BACNETSTACK_SRCS
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/abort.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/abort.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/access_rule.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/access_rule.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/alarm_ack.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/alarm_ack.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/apdu.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/arf.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/arf.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/assigned_access_rights.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/assigned_access_rights.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/authentication_factor.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/authentication_factor_format.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/authentication_factor_format.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/authentication_factor.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/awf.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/awf.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/bacaddr.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/bacaddr.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/bacapp.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/bacapp.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/bacdcode.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/bacdcode.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/bacdef.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/bacdevobjpropref.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/bacdevobjpropref.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/bacenum.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/bacerror.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/bacerror.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/bacint.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/bacint.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/bacprop.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/bacprop.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/bacpropstates.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/bacpropstates.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/bacreal.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/bacreal.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/bacstr.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/bacstr.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/bactext.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/bactext.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/bactimevalue.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/bactimevalue.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/bbmd6/h_bbmd6.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/bbmd6/h_bbmd6.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/bbmd6/vmac.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/bbmd6/vmac.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/binding/address.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/binding/address.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/npdu/h_npdu.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/npdu/h_npdu.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/npdu/h_routed_npdu.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/npdu/h_routed_npdu.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/npdu/s_router.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/npdu/s_router.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/object/access_credential.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/object/access_credential.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/object/access_door.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/object/access_door.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/object/access_point.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/object/access_point.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/object/access_rights.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/object/access_rights.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/object/access_user.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/object/access_user.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/object/access_zone.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/object/access_zone.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/object/ai.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/object/ai.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/object/ao.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/object/ao.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/object/av.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/object/av.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/object/bacfile.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/object/bacfile.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/object/bi.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/object/bi.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/object/bo.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/object/bo.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/object/bv.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/object/bv.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/object/channel.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/object/channel.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/object/client/device-client.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/object/command.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/object/command.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/object/credential_data_input.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/object/credential_data_input.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/object/csv.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/object/csv.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/object/device.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/object/device.h
    $<$<BOOL:${BAC_ROUTING}>:${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/object/gateway/gw_device.c>
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/object/iv.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/object/iv.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/object/lc.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/object/lc.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/object/lo.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/object/lo.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/object/lsp.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/object/lsp.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/object/Makefile
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/object/ms-input.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/object/ms-input.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/object/mso.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/object/mso.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/object/msv.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/object/msv.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/object/nc.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/object/nc.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/object/netport.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/object/netport.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/object/objects.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/object/objects.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/object/osv.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/object/osv.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/object/piv.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/object/piv.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/object/schedule.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/object/schedule.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/object/trendlog.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/object/trendlog.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/h_alarm_ack.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/h_alarm_ack.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/h_apdu.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/h_apdu.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/h_arf_a.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/h_arf_a.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/h_arf.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/h_arf.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/h_awf.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/h_awf.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/h_ccov.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/h_ccov.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/h_cov.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/h_cov.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/h_dcc.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/h_dcc.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/h_gas_a.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/h_gas_a.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/h_get_alarm_sum.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/h_get_alarm_sum.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/h_getevent_a.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/h_getevent_a.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/h_getevent.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/h_getevent.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/h_iam.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/h_iam.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/h_ihave.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/h_ihave.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/h_lso.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/h_lso.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/h_noserv.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/h_noserv.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/h_rd.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/h_rd.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/h_rp_a.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/h_rp_a.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/h_rp.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/h_rp.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/h_rpm_a.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/h_rpm_a.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/h_rpm.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/h_rpm.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/h_rr_a.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/h_rr_a.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/h_rr.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/h_rr.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/h_ts.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/h_ts.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/h_ucov.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/h_ucov.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/h_upt.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/h_upt.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/h_whohas.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/h_whohas.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/h_whois.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/h_whois.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/h_wp.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/h_wp.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/h_wpm.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/h_wpm.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/s_abort.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/s_abort.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/s_ack_alarm.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/s_ack_alarm.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/s_arfs.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/s_arfs.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/s_awfs.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/s_awfs.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/s_cevent.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/s_cevent.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/s_cov.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/s_cov.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/s_dcc.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/s_dcc.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/s_error.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/s_error.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/s_get_alarm_sum.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/s_get_alarm_sum.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/s_get_event.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/s_getevent.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/s_get_event.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/s_getevent.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/s_iam.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/s_iam.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/s_ihave.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/s_ihave.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/s_lso.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/s_lso.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/s_rd.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/s_rd.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/s_readrange.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/s_readrange.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/s_rp.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/s_rp.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/s_rpm.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/s_rpm.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/s_ts.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/s_ts.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/s_uevent.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/s_uevent.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/s_upt.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/s_upt.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/s_whohas.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/s_whohas.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/s_whois.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/s_whois.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/s_wp.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/s_wp.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/s_wpm.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/service/s_wpm.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/services.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/sys/bigend.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/sys/bigend.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/sys/debug.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/sys/debug.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/sys/fifo.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/sys/fifo.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/sys/filename.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/sys/filename.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/sys/key.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/sys/key.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/sys/keylist.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/sys/keylist.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/sys/mstimer.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/sys/mstimer.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/sys/ringbuf.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/sys/ringbuf.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/sys/sbuf.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/sys/sbuf.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/tsm/tsm.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/basic/tsm/tsm.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/bits.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/bytes.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/config.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/cov.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/cov.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/credential_authentication_factor.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/credential_authentication_factor.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/datalink/arcnet.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/datalink/bacsec.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/datalink/bacsec.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/datalink/bip6.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/datalink/bip.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/datalink/bip.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/datalink/bvlc6.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/datalink/bvlc6.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/datalink/bvlc.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/datalink/bvlc.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/datalink/crc.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/datalink/crc.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/datalink/datalink.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/datalink/datalink.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/datalink/dlenv.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/datalink/dlenv.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/datalink/dlmstp.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/datalink/ethernet.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/datalink/mstp.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/datalink/mstpdef.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/datalink/mstp.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/datalink/mstptext.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/datalink/mstptext.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/datetime.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/datetime.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/dcc.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/dcc.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/event.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/event.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/get_alarm_sum.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/get_alarm_sum.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/getevent.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/getevent.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/iam.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/iam.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/ihave.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/ihave.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/indtext.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/indtext.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/lighting.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/lighting.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/lso.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/lso.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/memcopy.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/memcopy.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/npdu.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/npdu.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/property.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/property.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/proplist.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/proplist.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/ptransfer.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/ptransfer.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/rd.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/rd.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/readrange.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/readrange.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/reject.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/reject.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/rp.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/rp.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/rpm.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/rpm.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/timestamp.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/timestamp.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/timesync.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/timesync.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/version.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/whohas.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/whohas.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/whois.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/whois.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/wp.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/wp.h
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/wpm.c
    ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/wpm.h)

if(LINUX)
  list(APPEND BACNETSTACK_SRCS
              ${CMAKE_CURRENT_LIST_DIR}/ports/linux/arcnet.c
              # ${CMAKE_CURRENT_LIST_DIR}/ports/linux/bip6.c
              # ${CMAKE_CURRENT_LIST_DIR}/ports/linux/rx_fsm.c
              ${CMAKE_CURRENT_LIST_DIR}/ports/linux/bip-init.c
              ${CMAKE_CURRENT_LIST_DIR}/ports/linux/dlmstp.c
              ${CMAKE_CURRENT_LIST_DIR}/ports/linux/dlmstp_linux.c
              ${CMAKE_CURRENT_LIST_DIR}/ports/linux/dlmstp_linux.h
              ${CMAKE_CURRENT_LIST_DIR}/ports/linux/ethernet.c
              ${CMAKE_CURRENT_LIST_DIR}/ports/linux/mstpsnap.c
              ${CMAKE_CURRENT_LIST_DIR}/ports/linux/net.h
              ${CMAKE_CURRENT_LIST_DIR}/ports/linux/rs485.c
              ${CMAKE_CURRENT_LIST_DIR}/ports/linux/rs485.h
              ${CMAKE_CURRENT_LIST_DIR}/ports/linux/timer.c
              ${CMAKE_CURRENT_LIST_DIR}/ports/linux/timer.h)
elseif(WIN32)
  list(APPEND BACNETSTACK_SRCS
              ${CMAKE_CURRENT_LIST_DIR}/ports/win32/bacport.h
              ${CMAKE_CURRENT_LIST_DIR}/ports/win32/bip6.c
              ${CMAKE_CURRENT_LIST_DIR}/ports/win32/bip-init.c
              ${CMAKE_CURRENT_LIST_DIR}/ports/win32/datetime-init.c
              ${CMAKE_CURRENT_LIST_DIR}/ports/win32/dlmstp.c
              ${CMAKE_CURRENT_LIST_DIR}/ports/win32/dlmstp.cbp
              ${CMAKE_CURRENT_LIST_DIR}/ports/win32/dlmstp-mm.c
              ${CMAKE_CURRENT_LIST_DIR}/ports/win32/ethernet.c
              # ${CMAKE_CURRENT_LIST_DIR}/ports/win32/main.c
              ${CMAKE_CURRENT_LIST_DIR}/ports/win32/mstimer-init.c
              ${CMAKE_CURRENT_LIST_DIR}/ports/win32/rs485.c
              ${CMAKE_CURRENT_LIST_DIR}/ports/win32/rs485.h
              ${CMAKE_CURRENT_LIST_DIR}/ports/win32/rx_fsm.c
              ${CMAKE_CURRENT_LIST_DIR}/ports/win32/rx_fsm.cbp
              ${CMAKE_CURRENT_LIST_DIR}/ports/win32/stdbool.h
              ${CMAKE_CURRENT_LIST_DIR}/ports/win32/stdint.h)
endif()

add_library(${PROJECT_NAME} ${BACNETSTACK_SRCS})
target_include_directories(${PROJECT_NAME}
                           PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/src/>
                                  $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/src/bacnet/>
                                  $<INSTALL_INTERFACE:include/bacnet/>
                                  $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/ports/linux/>)
target_compile_definitions(${PROJECT_NAME}
                           PUBLIC
                           $<$<BOOL:${BAC_ROUTING}>:BAC_ROUTING>)

if(BACNET_STACK_BUILD_APPS)
  message(STATUS "BACNET: compilinng also apps")
  add_executable(abort ${CMAKE_CURRENT_LIST_DIR}/apps/abort/main.c)
  target_link_libraries(abort PRIVATE ${PROJECT_NAME})
  add_executable(dcc ${CMAKE_CURRENT_LIST_DIR}/apps/dcc/main.c)
  target_link_libraries(dcc PRIVATE ${PROJECT_NAME})
  add_executable(epics ${CMAKE_CURRENT_LIST_DIR}/apps/epics/main.c)
  target_link_libraries(epics PRIVATE ${PROJECT_NAME})
  add_executable(error ${CMAKE_CURRENT_LIST_DIR}/apps/error/main.c)
  target_link_libraries(error PRIVATE ${PROJECT_NAME})
  add_executable(gateway ${CMAKE_CURRENT_LIST_DIR}/apps/gateway/main.c)
  target_link_libraries(gateway PRIVATE ${PROJECT_NAME})
  add_executable(getevent ${CMAKE_CURRENT_LIST_DIR}/apps/getevent/main.c)
  target_link_libraries(getevent PRIVATE ${PROJECT_NAME})
  add_executable(iam ${CMAKE_CURRENT_LIST_DIR}/apps/iam/main.c)
  target_link_libraries(iam PRIVATE ${PROJECT_NAME})
  add_executable(iamrouter ${CMAKE_CURRENT_LIST_DIR}/apps/iamrouter/main.c)
  target_link_libraries(iamrouter PRIVATE ${PROJECT_NAME})
  add_executable(initrouter ${CMAKE_CURRENT_LIST_DIR}/apps/initrouter/main.c)
  target_link_libraries(initrouter PRIVATE ${PROJECT_NAME})
  add_executable(mstpcap ${CMAKE_CURRENT_LIST_DIR}/apps/mstpcap/main.c)
  target_link_libraries(mstpcap PRIVATE ${PROJECT_NAME})
  add_executable(mstpcrc ${CMAKE_CURRENT_LIST_DIR}/apps/mstpcrc/main.c)
  target_link_libraries(mstpcrc PRIVATE ${PROJECT_NAME})
  add_executable(piface ${CMAKE_CURRENT_LIST_DIR}/apps/piface/main.c)
  target_link_libraries(piface PRIVATE ${PROJECT_NAME})
  add_executable(ptransfer ${CMAKE_CURRENT_LIST_DIR}/apps/ptransfer/main.c)
  target_link_libraries(ptransfer PRIVATE ${PROJECT_NAME})
  add_executable(readbdt ${CMAKE_CURRENT_LIST_DIR}/apps/readbdt/main.c)
  target_link_libraries(readbdt PRIVATE ${PROJECT_NAME})
  add_executable(readfile ${CMAKE_CURRENT_LIST_DIR}/apps/readfile/main.c)
  target_link_libraries(readfile PRIVATE ${PROJECT_NAME})
  add_executable(readprop ${CMAKE_CURRENT_LIST_DIR}/apps/readprop/main.c)
  target_link_libraries(readprop PRIVATE ${PROJECT_NAME})
  add_executable(readpropm ${CMAKE_CURRENT_LIST_DIR}/apps/readpropm/main.c)
  target_link_libraries(readpropm PRIVATE ${PROJECT_NAME})
  add_executable(readrange ${CMAKE_CURRENT_LIST_DIR}/apps/readrange/main.c)
  target_link_libraries(readrange PRIVATE ${PROJECT_NAME})
  add_executable(reinit ${CMAKE_CURRENT_LIST_DIR}/apps/reinit/main.c)
  target_link_libraries(reinit PRIVATE ${PROJECT_NAME})
  add_executable(router ${CMAKE_CURRENT_LIST_DIR}/apps/router/main.c)
  target_link_libraries(router PRIVATE ${PROJECT_NAME})
  add_executable(router-ipv6 ${CMAKE_CURRENT_LIST_DIR}/apps/router-ipv6/main.c)
  target_link_libraries(router PRIVATE ${PROJECT_NAME})
  add_executable(scov ${CMAKE_CURRENT_LIST_DIR}/apps/scov/main.c)
  target_link_libraries(scov PRIVATE ${PROJECT_NAME})
  add_executable(server ${CMAKE_CURRENT_LIST_DIR}/apps/server/main.c)
  target_link_libraries(server PRIVATE ${PROJECT_NAME})
  add_executable(timesync ${CMAKE_CURRENT_LIST_DIR}/apps/timesync/main.c)
  target_link_libraries(timesync PRIVATE ${PROJECT_NAME})
  add_executable(ucov ${CMAKE_CURRENT_LIST_DIR}/apps/ucov/main.c)
  target_link_libraries(ucov PRIVATE ${PROJECT_NAME})
  add_executable(uevent ${CMAKE_CURRENT_LIST_DIR}/apps/uevent/main.c)
  target_link_libraries(uevent PRIVATE ${PROJECT_NAME})
  add_executable(uptransfer ${CMAKE_CURRENT_LIST_DIR}/apps/uptransfer/main.c)
  target_link_libraries(uptransfer PRIVATE ${PROJECT_NAME})
  add_executable(whohas ${CMAKE_CURRENT_LIST_DIR}/apps/whohas/main.c)
  target_link_libraries(whohas PRIVATE ${PROJECT_NAME})
  add_executable(whois ${CMAKE_CURRENT_LIST_DIR}/apps/whois/main.c)
  target_link_libraries(whois PRIVATE ${PROJECT_NAME})
  add_executable(whoisrouter ${CMAKE_CURRENT_LIST_DIR}/apps/whoisrouter/main.c)
  target_link_libraries(whoisrouter PRIVATE ${PROJECT_NAME})
  add_executable(writefile ${CMAKE_CURRENT_LIST_DIR}/apps/writefile/main.c)
  target_link_libraries(writefile PRIVATE ${PROJECT_NAME})
  add_executable(writeprop ${CMAKE_CURRENT_LIST_DIR}/apps/writeprop/main.c)
  target_link_libraries(writeprop PRIVATE ${PROJECT_NAME})
  add_executable(writepropm ${CMAKE_CURRENT_LIST_DIR}/apps/writepropm/main.c)
  target_link_libraries(writepropm PRIVATE ${PROJECT_NAME})
endif()

install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/src/bacnet/ DESTINATION include/bacnet FILES_MATCHING PATTERN "*.h")
install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION bin
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib PERMISSIONS WORLD_READ)
