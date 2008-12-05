LOGFILE = test.log

all: abort address arf awf bacapp bacdcode bacerror bacint \
	bacstr cov crc datetime dcc fifo filename iam ihave \
	indtext keylist key mstp npdu rd reject ringbuf rp \
	rpm sbuf timesync whohas whois wp

clean:
	rm ${LOGFILE}

logfile:
	touch ${LOGFILE}

abort: logfile test/abort.mak
	( cd test ; make -f abort.mak clean )
	( cd test ; make -f abort.mak )
	( ./test/abort >> ${LOGFILE} )
	( cd test ; make -f abort.mak clean )

address: logfile test/address.mak
	( cd test ; make -f address.mak clean )
	( cd test ; make -f address.mak )
	( ./test/address >> ${LOGFILE} )
	( cd test ; make -f address.mak clean )

arf: logfile test/arf.mak
	( cd test ; make -f arf.mak clean )
	( cd test ; make -f arf.mak )
	( ./test/arf >> ${LOGFILE} )
	( cd test ; make -f arf.mak clean )

awf: logfile test/awf.mak
	( cd test ; make -f awf.mak clean )
	( cd test ; make -f awf.mak )
	( ./test/awf >> ${LOGFILE} )
	( cd test ; make -f awf.mak clean )

bacapp: logfile test/bacapp.mak
	( cd test ; make -f bacapp.mak clean )
	( cd test ; make -f bacapp.mak )
	( ./test/bacapp >> ${LOGFILE} )
	( cd test ; make -f bacapp.mak clean )

bacdcode: logfile test/bacdcode.mak
	( cd test ; make -f bacdcode.mak clean )
	( cd test ; make -f bacdcode.mak )
	( ./test/bacdcode >> ${LOGFILE} )
	( cd test ; make -f bacdcode.mak clean )

bacerror: logfile test/bacerror.mak
	( cd test ; make -f bacerror.mak clean )
	( cd test ; make -f bacerror.mak )
	( ./test/bacerror >> ${LOGFILE} )
	( cd test ; make -f bacerror.mak clean )

bacint: logfile test/bacint.mak
	( cd test ; make -f bacint.mak clean )
	( cd test ; make -f bacint.mak )
	( ./test/bacint >> ${LOGFILE} )
	( cd test ; make -f bacint.mak clean )

bacstr: logfile test/bacstr.mak
	( cd test ; make -f bacstr.mak clean )
	( cd test ; make -f bacstr.mak )
	( ./test/bacstr >> ${LOGFILE} )
	( cd test ; make -f bacstr.mak clean )

cov: logfile test/cov.mak
	( cd test ; make -f cov.mak clean )
	( cd test ; make -f cov.mak )
	( ./test/cov >> ${LOGFILE} )
	( cd test ; make -f cov.mak clean )

crc: logfile test/crc.mak
	( cd test ; make -f crc.mak clean )
	( cd test ; make -f crc.mak )
	( ./test/crc >> ${LOGFILE} )
	( cd test ; make -f crc.mak clean )

datetime: logfile test/datetime.mak
	( cd test ; make -f datetime.mak clean )
	( cd test ; make -f datetime.mak )
	( ./test/datetime >> ${LOGFILE} )
	( cd test ; make -f datetime.mak clean )

dcc: logfile test/dcc.mak
	( cd test ; make -f dcc.mak clean )
	( cd test ; make -f dcc.mak )
	( ./test/dcc >> ${LOGFILE} )
	( cd test ; make -f dcc.mak clean )

filename: logfile test/filename.mak
	( cd test ; make -f filename.mak clean )
	( cd test ; make -f filename.mak )
	( ./test/filename >> ${LOGFILE} )
	( cd test ; make -f filename.mak clean )

fifo: logfile test/fifo.mak
	( cd test ; make -f fifo.mak clean )
	( cd test ; make -f fifo.mak )
	( ./test/fifo >> ${LOGFILE} )
	( cd test ; make -f fifo.mak clean )

iam: logfile test/iam.mak
	( cd test ; make -f iam.mak clean )
	( cd test ; make -f iam.mak )
	( ./test/iam >> ${LOGFILE} )
	( cd test ; make -f iam.mak clean )

ihave: logfile test/ihave.mak
	( cd test ; make -f ihave.mak clean )
	( cd test ; make -f ihave.mak )
	( ./test/ihave >> ${LOGFILE} )
	( cd test ; make -f ihave.mak clean )

indtext: logfile test/indtext.mak
	( cd test ; make -f indtext.mak clean )
	( cd test ; make -f indtext.mak )
	( ./test/indtext >> ${LOGFILE} )
	( cd test ; make -f indtext.mak clean )

keylist: logfile test/keylist.mak
	( cd test ; make -f keylist.mak clean )
	( cd test ; make -f keylist.mak )
	( ./test/keylist >> ${LOGFILE} )
	( cd test ; make -f keylist.mak clean )

key: logfile test/key.mak
	( cd test ; make -f key.mak clean )
	( cd test ; make -f key.mak )
	( ./test/key >> ${LOGFILE} )
	( cd test ; make -f key.mak clean )

mstp: logfile test/mstp.mak
	( cd test ; make -f mstp.mak clean )
	( cd test ; make -f mstp.mak )
	( ./test/mstp >> ${LOGFILE} )
	( cd test ; make -f mstp.mak clean )

memcopy: logfile test/memcopy.mak
	( cd test ; make -f memcopy.mak clean )
	( cd test ; make -f memcopy.mak )
	( ./test/memcopy >> ${LOGFILE} )
	( cd test ; make -f memcopy.mak clean )

npdu: logfile test/npdu.mak
	( cd test ; make -f npdu.mak clean )
	( cd test ; make -f npdu.mak )
	( ./test/npdu >> ${LOGFILE} )
	( cd test ; make -f npdu.mak clean )

rd: logfile test/rd.mak
	( cd test ; make -f rd.mak clean )
	( cd test ; make -f rd.mak )
	( ./test/rd >> ${LOGFILE} )
	( cd test ; make -f rd.mak clean )

reject: logfile test/reject.mak
	( cd test ; make -f reject.mak clean )
	( cd test ; make -f reject.mak )
	( ./test/reject >> ${LOGFILE} )
	( cd test ; make -f reject.mak clean )

ringbuf: logfile test/ringbuf.mak
	( cd test ; make -f ringbuf.mak clean )
	( cd test ; make -f ringbuf.mak )
	( ./test/ringbuf >> ${LOGFILE} )
	( cd test ; make -f ringbuf.mak clean )

rp: logfile test/rp.mak
	( cd test ; make -f rp.mak clean )
	( cd test ; make -f rp.mak )
	( ./test/rp >> ${LOGFILE} )
	( cd test ; make -f rp.mak clean )

rpm: logfile test/rpm.mak
	( cd test ; make -f rpm.mak clean )
	( cd test ; make -f rpm.mak )
	( ./test/rpm >> ${LOGFILE} )
	( cd test ; make -f rpm.mak clean )

sbuf: logfile test/sbuf.mak
	( cd test ; make -f sbuf.mak clean )
	( cd test ; make -f sbuf.mak )
	( ./test/sbuf >> ${LOGFILE} )
	( cd test ; make -f sbuf.mak clean )

timesync: logfile test/timesync.mak
	( cd test ; make -f timesync.mak clean )
	( cd test ; make -f timesync.mak )
	( ./test/timesync >> ${LOGFILE} )
	( cd test ; make -f timesync.mak clean )

whohas: logfile test/whohas.mak
	( cd test ; make -f whohas.mak clean )
	( cd test ; make -f whohas.mak )
	( ./test/whohas >> ${LOGFILE} )
	( cd test ; make -f whohas.mak clean )

whois: logfile test/whois.mak
	( cd test ; make -f whois.mak clean )
	( cd test ; make -f whois.mak )
	( ./test/whois >> ${LOGFILE} )
	( cd test ; make -f whois.mak clean )

wp: logfile test/wp.mak
	( cd test ; make -f wp.mak clean )
	( cd test ; make -f wp.mak )
	( ./test/wp >> ${LOGFILE} )
	( cd test ; make -f wp.mak clean )

# Objects
ai: logfile demo/object/ai.mak
	( cd demo/object ; make -f ai.mak clean )
	( cd demo/object ; make -f ai.mak )
	( ./demo/object/analog_input >> ${LOGFILE} )
	( cd demo/object ; make -f ai.mak clean )

ao: logfile demo/object/ao.mak
	( cd demo/object ; make -f ao.mak clean )
	( cd demo/object ; make -f ao.mak )
	( ./demo/object/analog_output >> ${LOGFILE} )
	( cd demo/object ; make -f ao.mak clean )

av: logfile demo/object/av.mak
	( cd demo/object ; make -f av.mak clean )
	( cd demo/object ; make -f av.mak )
	( ./demo/object/analog_value >> ${LOGFILE} )
	( cd demo/object ; make -f av.mak clean )

bi: logfile demo/object/bi.mak
	( cd demo/object ; make -f bi.mak clean )
	( cd demo/object ; make -f bi.mak )
	( ./demo/object/binary_input >> ${LOGFILE} )
	( cd demo/object ; make -f bi.mak clean )

bo: logfile demo/object/bo.mak
	( cd demo/object ; make -f bo.mak clean )
	( cd demo/object ; make -f bo.mak )
	( ./demo/object/binar_output >> ${LOGFILE} )
	( cd demo/object ; make -f bo.mak clean )

bv: logfile demo/object/bv.mak
	( cd demo/object ; make -f bv.mak clean )
	( cd demo/object ; make -f bv.mak )
	( ./demo/object/binary_value >> ${LOGFILE} )
	( cd demo/object ; make -f bv.mak clean )

device: logfile demo/object/device.mak
	( cd demo/object ; make -f device.mak clean )
	( cd demo/object ; make -f device.mak )
	( ./demo/object/device >> ${LOGFILE} )
	( cd demo/object ; make -f device.mak clean )

lc: logfile demo/object/lc.mak
	( cd demo/object ; make -f lc.mak clean )
	( cd demo/object ; make -f lc.mak )
	( ./demo/object/load_control >> ${LOGFILE} )
	( cd demo/object ; make -f lc.mak clean )

lo: logfile demo/object/lo.mak
	( cd demo/object ; make -f lo.mak clean )
	( cd demo/object ; make -f lo.mak )
	( ./demo/object/lighting_output >> ${LOGFILE} )
	( cd demo/object ; make -f lo.mak clean )

lsp: logfile demo/object/lsp.mak
	( cd demo/object ; make -f lsp.mak clean )
	( cd demo/object ; make -f lsp.mak )
	( ./demo/object/life_safety_point >> ${LOGFILE} )
	( cd demo/object ; make -f lsp.mak clean )

mso: logfile demo/object/mso.mak
	( cd demo/object ; make -f mso.mak clean )
	( cd demo/object ; make -f mso.mak )
	( ./demo/object/multistate_output >> ${LOGFILE} )
	( cd demo/object ; make -f mso.mak clean )

