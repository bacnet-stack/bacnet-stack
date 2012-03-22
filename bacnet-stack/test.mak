LOGFILE = test.log

all: abort address arf awf bacapp bacdcode bacerror bacint bacstr \
	cov crc datetime dcc event filename fifo getevent iam ihave \
	indtext keylist key memcopy npdu ptransfer \
	rd reject ringbuf rp rpm sbuf timesync \
	whohas whois wp objects

clean:
	rm ${LOGFILE}

logfile:
	touch ${LOGFILE}

abort: logfile test/abort.mak
	make -C test -f abort.mak clean all
	( ./test/abort >> ${LOGFILE} )
	make -C test -f abort.mak clean

address: logfile test/address.mak
	make -C test -f address.mak clean all
	( ./test/address >> ${LOGFILE} )
	make -C test -f address.mak clean

arf: logfile test/arf.mak
	make -C test -f arf.mak clean all
	( ./test/arf >> ${LOGFILE} )
	make -C test -f arf.mak clean

awf: logfile test/awf.mak
	make -C test -f awf.mak clean all
	( ./test/awf >> ${LOGFILE} )
	make -C test -f awf.mak clean

bacapp: logfile test/bacapp.mak
	make -C test -f bacapp.mak clean all
	( ./test/bacapp >> ${LOGFILE} )
	make -C test -f bacapp.mak clean

bacdcode: logfile test/bacdcode.mak
	make -C test -f bacdcode.mak clean all
	( ./test/bacdcode >> ${LOGFILE} )
	make -C test -f bacdcode.mak clean

bacerror: logfile test/bacerror.mak
	make -C test -f bacerror.mak clean all
	( ./test/bacerror >> ${LOGFILE} )
	make -C test -f bacerror.mak clean

bacint: logfile test/bacint.mak
	make -C test -f bacint.mak clean all
	( ./test/bacint >> ${LOGFILE} )
	make -C test -f bacint.mak clean

bacstr: logfile test/bacstr.mak
	make -C test -f bacstr.mak clean all
	( ./test/bacstr >> ${LOGFILE} )
	make -C test -f bacstr.mak clean

cov: logfile test/cov.mak
	make -C test -f cov.mak clean all
	( ./test/cov >> ${LOGFILE} )
	make -C test -f cov.mak clean

crc: logfile test/crc.mak
	make -C test -f crc.mak clean all
	( ./test/crc >> ${LOGFILE} )
	make -C test -f crc.mak clean

datetime: logfile test/datetime.mak
	make -C test -f datetime.mak clean all
	( ./test/datetime >> ${LOGFILE} )
	make -C test -f datetime.mak clean

dcc: logfile test/dcc.mak
	make -C test -f dcc.mak clean all
	( ./test/dcc >> ${LOGFILE} )
	make -C test -f dcc.mak clean

event: logfile test/event.mak
	make -C test -f event.mak clean all
	( ./test/event >> ${LOGFILE} )
	make -C test -f event.mak clean

filename: logfile test/filename.mak
	make -C test -f filename.mak clean all
	( ./test/filename >> ${LOGFILE} )
	make -C test -f filename.mak clean

fifo: logfile test/fifo.mak
	make -C test -f fifo.mak clean all
	( ./test/fifo >> ${LOGFILE} )
	make -C test -f fifo.mak clean

getevent: logfile test/getevent.mak
	make -C test -f getevent.mak clean all
	( ./test/getevent >> ${LOGFILE} )
	make -C test -f getevent.mak clean

iam: logfile test/iam.mak
	make -C test -f iam.mak clean all
	( ./test/iam >> ${LOGFILE} )
	make -C test -f iam.mak clean

ihave: logfile test/ihave.mak
	make -C test -f ihave.mak clean all
	( ./test/ihave >> ${LOGFILE} )
	make -C test -f ihave.mak clean

indtext: logfile test/indtext.mak
	make -C test -f indtext.mak clean all
	( ./test/indtext >> ${LOGFILE} )
	make -C test -f indtext.mak clean

keylist: logfile test/keylist.mak
	make -C test -f keylist.mak clean all
	( ./test/keylist >> ${LOGFILE} )
	make -C test -f keylist.mak clean

key: logfile test/key.mak
	make -C test -f key.mak clean all
	( ./test/key >> ${LOGFILE} )
	make -C test -f key.mak clean

memcopy: logfile test/memcopy.mak
	make -C test -f memcopy.mak clean all
	( ./test/memcopy >> ${LOGFILE} )
	make -C test -f memcopy.mak clean

npdu: logfile test/npdu.mak
	make -C test -f npdu.mak clean all
	( ./test/npdu >> ${LOGFILE} )
	make -C test -f npdu.mak clean

ptransfer: logfile test/ptransfer.mak
	make -C test -f ptransfer.mak clean all
	( ./test/ptransfer >> ${LOGFILE} )
	make -C test -f ptransfer.mak clean

rd: logfile test/rd.mak
	make -C test -f rd.mak clean all
	( ./test/rd >> ${LOGFILE} )
	make -C test -f rd.mak clean

reject: logfile test/reject.mak
	make -C test -f reject.mak clean all
	( ./test/reject >> ${LOGFILE} )
	make -C test -f reject.mak clean

ringbuf: logfile test/ringbuf.mak
	make -C test -f ringbuf.mak clean all
	( ./test/ringbuf >> ${LOGFILE} )
	make -C test -f ringbuf.mak clean

rp: logfile test/rp.mak
	make -C test -f rp.mak clean all
	( ./test/rp >> ${LOGFILE} )
	make -C test -f rp.mak clean

rpm: logfile test/rpm.mak
	make -C test -f rpm.mak clean all
	( ./test/rpm >> ${LOGFILE} )
	make -C test -f rpm.mak clean

sbuf: logfile test/sbuf.mak
	make -C test -f sbuf.mak clean all
	( ./test/sbuf >> ${LOGFILE} )
	make -C test -f sbuf.mak clean

timesync: logfile test/timesync.mak
	make -C test -f timesync.mak clean all
	( ./test/timesync >> ${LOGFILE} )
	make -C test -f timesync.mak clean

whohas: logfile test/whohas.mak
	make -C test -f whohas.mak clean all
	( ./test/whohas >> ${LOGFILE} )
	make -C test -f whohas.mak clean

whois: logfile test/whois.mak
	make -C test -f whois.mak clean all
	( ./test/whois >> ${LOGFILE} )
	make -C test -f whois.mak clean

wp: logfile test/wp.mak
	make -C test -f wp.mak clean all
	( ./test/wp >> ${LOGFILE} )
	make -C test -f wp.mak clean

objects: ai ao av bi bo bv csv lc lo lso lsp mso msv ms-input

ai: logfile demo/object/ai.mak
	make -C demo/object -f ai.mak clean all
	( ./demo/object/analog_input >> ${LOGFILE} )
	make -C demo/object -f ai.mak clean

ao: logfile demo/object/ao.mak
	make -C demo/object -f ao.mak clean all
	( ./demo/object/analog_output >> ${LOGFILE} )
	make -C demo/object -f ao.mak clean

av: logfile demo/object/av.mak
	make -C demo/object -f av.mak clean all
	( ./demo/object/analog_value >> ${LOGFILE} )
	make -C demo/object -f av.mak clean

bi: logfile demo/object/bi.mak
	make -C demo/object -f bi.mak clean all
	make -C demo/object -f bi.mak clean

bo: logfile demo/object/bo.mak
	make -C demo/object -f bo.mak clean all
	( ./demo/object/binary_output >> ${LOGFILE} )
	make -C demo/object -f bo.mak clean

bv: logfile demo/object/bv.mak
	make -C demo/object -f bv.mak clean all
	( ./demo/object/binary_value >> ${LOGFILE} )
	make -C demo/object -f bv.mak clean

csv: logfile demo/object/csv.mak
	make -C demo/object -f csv.mak clean all
	( ./demo/object/characterstring_value >> ${LOGFILE} )
	make -C demo/object -f csv.mak clean

device: logfile demo/object/device.mak
	make -C demo/object -f device.mak clean all
	( ./demo/object/device >> ${LOGFILE} )
	make -C demo/object -f device.mak clean

lc: logfile demo/object/lc.mak
	make -C demo/object -f lc.mak clean all
	( ./demo/object/load_control >> ${LOGFILE} )
	make -C demo/object -f lc.mak clean

lo: logfile demo/object/lo.mak
	make -C demo/object -f lo.mak clean all
	( ./demo/object/lighting_output >> ${LOGFILE} )
	make -C demo/object -f lo.mak clean

lso: logfile test/lso.mak
	make -C test -f lso.mak clean all
	( ./test/lso >> ${LOGFILE} )
	make -C test -f lso.mak clean

lsp: logfile demo/object/lsp.mak
	make -C demo/object -f lsp.mak clean all
	( ./demo/object/life_safety_point >> ${LOGFILE} )
	make -C demo/object -f lsp.mak clean

ms-input: logfile demo/object/ms-input.mak
	make -C demo/object -f ms-input.mak clean all
	( ./demo/object/multistate_input >> ${LOGFILE} )
	make -C demo/object -f ms-input.mak clean

mso: logfile demo/object/mso.mak
	make -C demo/object -f mso.mak clean all
	( ./demo/object/multistate_output >> ${LOGFILE} )
	make -C demo/object -f mso.mak clean

msv: logfile demo/object/msv.mak
	make -C demo/object -f msv.mak clean all
	( ./demo/object/multistate_value >> ${LOGFILE} )
	make -C demo/object -f msv.mak clean
