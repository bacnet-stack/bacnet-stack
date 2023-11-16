cmake_minimum_required(VERSION 3.15)

add_library(BACnet_Stack STATIC)

set(BACNET_STACK_PATH "../../../src")
set(BACNET_CORE ${BACNET_STACK_PATH}/bacnet)
set(BACNET_BASIC ${BACNET_STACK_PATH}/bacnet/basic)

target_include_directories(BACnet_Stack PUBLIC
    ${BACNET_STACK_PATH}
)

target_sources(BACnet_Stack PRIVATE
    ${BACNET_BASIC}/object/netport.c
	${BACNET_BASIC}/service/h_dcc.c
	${BACNET_BASIC}/service/h_apdu.c
	${BACNET_BASIC}/service/h_rd.c
	${BACNET_BASIC}/service/h_rp.c
	${BACNET_BASIC}/service/h_rpm.c
	${BACNET_BASIC}/service/h_whohas.c
	${BACNET_BASIC}/service/h_whois.c
	${BACNET_BASIC}/service/h_wp.c
	${BACNET_BASIC}/service/h_noserv.c
	${BACNET_BASIC}/service/s_iam.c
	${BACNET_BASIC}/service/s_ihave.c
	${BACNET_BASIC}/npdu/h_npdu.c
	${BACNET_BASIC}/tsm/tsm.c
	${BACNET_BASIC}/sys/days.c
	${BACNET_BASIC}/sys/debug.c
	${BACNET_BASIC}/sys/fifo.c
	${BACNET_BASIC}/sys/mstimer.c
	${BACNET_BASIC}/sys/ringbuf.c
	${BACNET_CORE}/datalink/cobs.c
	${BACNET_CORE}/datalink/crc.c
	${BACNET_CORE}/datalink/mstp.c
	${BACNET_CORE}/datalink/mstptext.c
	${BACNET_CORE}/abort.c
	${BACNET_CORE}/bacaddr.c
	${BACNET_CORE}/bacapp.c
	${BACNET_CORE}/bacdcode.c
	${BACNET_CORE}/bacdest.c
	${BACNET_CORE}/bacdevobjpropref.c
	${BACNET_CORE}/bacerror.c
	${BACNET_CORE}/bacint.c
	${BACNET_CORE}/bacreal.c
	${BACNET_CORE}/bacstr.c
	${BACNET_CORE}/datalink/crc.c
	${BACNET_CORE}/datetime.c
	${BACNET_CORE}/dcc.c
	${BACNET_CORE}/iam.c
	${BACNET_CORE}/ihave.c
	${BACNET_CORE}/indtext.c
	${BACNET_CORE}/hostnport.c
	${BACNET_CORE}/lighting.c
	${BACNET_CORE}/memcopy.c
	${BACNET_CORE}/npdu.c
	${BACNET_CORE}/proplist.c
	${BACNET_CORE}/rd.c
	${BACNET_CORE}/reject.c
	${BACNET_CORE}/rp.c
	${BACNET_CORE}/rpm.c
	${BACNET_CORE}/timestamp.c
	${BACNET_CORE}/weeklyschedule.c
	${BACNET_CORE}/dailyschedule.c
	${BACNET_CORE}/bactimevalue.c
	${BACNET_CORE}/whohas.c
	${BACNET_CORE}/whois.c
	${BACNET_CORE}/wp.c
)

