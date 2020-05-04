set(COMPONENT_SRCS "main.c"
                   "port/device.c"
				   "port/bo.c"
				   "port/rs485.c" #Kai
				   "port/dlmstp.c" # Kai 
				   #"port/bip_init.c" # For BACnet IP
				   #"port/ai.c"

				   "port/bacnet.c"
				   "port/led.c"

				   "bacnet/dcc.c"
				   "bacnet/basic/binding/address.c"
				   "bacnet/datalink/bvlc.c"
				   "bacnet/basic/tsm/tsm.c"
				   "bacnet/bacapp.c"
				   "bacnet/bacdcode.c"
				   "bacnet/bacstr.c"
				   "bacnet/datetime.c"
				   "bacnet/bacint.c"
				  	#"bacnet/bip.c"
				   "bacnet/basic/sys/debug.c"
				   "bacnet/bacreal.c"
				   "bacnet/lighting.c"
				   "bacnet/bacaddr.c"
				   "bacnet/bactext.c"
				   "bacnet/indtext.c"
				   "bacnet/npdu.c"
				   "bacnet/iam.c"
				   "bacnet/abort.c"
				   "bacnet/reject.c"
				   "bacnet/proplist.c"
				   
				   "bacnet/rp.c"
				   "bacnet/bacerror.c"
				   			   
				   "bacnet/basic/service/h_whois.c"
				   "bacnet/basic/service/h_noserv.c"
				   "bacnet/basic/service/h_rp.c"
				   "bacnet/basic/service/h_rpm.c"
				   "bacnet/basic/service/h_wp.c"
				   "bacnet/basic/service/h_cov.c"
				   "bacnet/basic/service/h_apdu.c"
				   "bacnet/basic/service/s_iam.c"
				   
				   "bacnet/basic/bbmd/h_bbmd.c"
				   "bacnet/datalink/datalink.c"
				   "bacnet/basic/npdu/h_npdu.c"

				   "bacnet/basic/sys/ringbuf.c"
				   "port/automac.c"
				   "bacnet/basic/sys/mstimer.c"
				   "bacnet/basic/sys/fifo.c"
				   "bacnet/datalink/crc.c"
				   "port/mstimer-init.c"
				   "bacnet/basic/service/h_whohas.c"
				   "bacnet/basic/service/h_rd.c"
				   "bacnet/basic/service/h_dcc.c"
				   "bacnet/whois.c"
				   "bacnet/rpm.c"
				   "bacnet/memcopy.c"
				   "bacnet/rd.c"
				   "bacnet/wp.c"
				   "bacnet/basic/service/s_ihave.c"
				   "bacnet/whohas.c"
				   "bacnet/ihave.c"
				   

				   )

set(COMPONENT_ADD_INCLUDEDIRS "." "port/include")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")

register_component()
