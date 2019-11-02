# Unit tests for the BACnet Stack project

LOGFILE = test.log

all: abort address arf awf bvlc6 bacapp bacdcode bacerror bacint bacstr \
	cov crc datetime dcc event filename fifo getevent iam ihave \
	indtext keylist key lighting lso memcopy npdu proplist ptransfer \
	rd reject ringbuf rp rpm sbuf timesync vmac \
	whohas whois wp

clean: logfile
	rm ${LOGFILE}

logfile:
	touch ${LOGFILE}

report:
	cat ${LOGFILE}

abort: logfile abort.mak
	$(MAKE) -j -f abort.mak clean all
	( ./abort >> ${LOGFILE} )
	$(MAKE) -j -f abort.mak clean

address: logfile address.mak
	$(MAKE) -j -f address.mak clean all
	( ./address >> ${LOGFILE} )
	$(MAKE) -j -f address.mak clean

arf: logfile arf.mak
	$(MAKE) -j -f arf.mak clean all
	( ./arf >> ${LOGFILE} )
	$(MAKE) -j -f arf.mak clean

awf: logfile awf.mak
	$(MAKE) -j -f awf.mak clean all
	( ./awf >> ${LOGFILE} )
	$(MAKE) -j -f awf.mak clean

bacapp: logfile bacapp.mak
	$(MAKE) -j -f bacapp.mak clean all
	( ./bacapp >> ${LOGFILE} )
	$(MAKE) -j -f bacapp.mak clean

bacdcode: logfile bacdcode.mak
	$(MAKE) -j -f bacdcode.mak clean all
	( ./bacdcode >> ${LOGFILE} )
	$(MAKE) -j -f bacdcode.mak clean

bacerror: logfile bacerror.mak
	$(MAKE) -j -f bacerror.mak clean all
	( ./bacerror >> ${LOGFILE} )
	$(MAKE) -j -f bacerror.mak clean

bacint: logfile bacint.mak
	$(MAKE) -j -f bacint.mak clean all
	( ./bacint >> ${LOGFILE} )
	$(MAKE) -j -f bacint.mak clean

bacstr: logfile bacstr.mak
	$(MAKE) -j -f bacstr.mak clean all
	( ./bacstr >> ${LOGFILE} )
	$(MAKE) -j -f bacstr.mak clean

bvlc6: logfile bvlc6.mak
	$(MAKE) -j -f bvlc6.mak clean all
	( ./bvlc6 >> ${LOGFILE} )
	$(MAKE) -j -f bvlc6.mak clean

cov: logfile cov.mak
	$(MAKE) -j -f cov.mak clean all
	( ./cov >> ${LOGFILE} )
	$(MAKE) -j -f cov.mak clean

crc: logfile crc.mak
	$(MAKE) -j -f crc.mak clean all
	( ./crc >> ${LOGFILE} )
	$(MAKE) -j -f crc.mak clean

datetime: logfile datetime.mak
	$(MAKE) -j -f datetime.mak clean all
	( ./datetime >> ${LOGFILE} )
	$(MAKE) -j -f datetime.mak clean

dcc: logfile dcc.mak
	$(MAKE) -j -f dcc.mak clean all
	( ./dcc >> ${LOGFILE} )
	$(MAKE) -j -f dcc.mak clean

event: logfile event.mak
	$(MAKE) -j -f event.mak clean all
	( ./event >> ${LOGFILE} )
	$(MAKE) -j -f event.mak clean

filename: logfile filename.mak
	$(MAKE) -j -f filename.mak clean all
	( ./filename >> ${LOGFILE} )
	$(MAKE) -j -f filename.mak clean

fifo: logfile fifo.mak
	$(MAKE) -j -f fifo.mak clean all
	( ./fifo >> ${LOGFILE} )
	$(MAKE) -j -f fifo.mak clean

getevent: logfile getevent.mak
	$(MAKE) -j -f getevent.mak clean all
	( ./getevent >> ${LOGFILE} )
	$(MAKE) -j -f getevent.mak clean

iam: logfile iam.mak
	$(MAKE) -j -f iam.mak clean all
	( ./iam >> ${LOGFILE} )
	$(MAKE) -j -f iam.mak clean

ihave: logfile ihave.mak
	$(MAKE) -j -f ihave.mak clean all
	( ./ihave >> ${LOGFILE} )
	$(MAKE) -j -f ihave.mak clean

indtext: logfile indtext.mak
	$(MAKE) -j -f indtext.mak clean all
	( ./indtext >> ${LOGFILE} )
	$(MAKE) -j -f indtext.mak clean

keylist: logfile keylist.mak
	$(MAKE) -j -f keylist.mak clean all
	( ./keylist >> ${LOGFILE} )
	$(MAKE) -j -f keylist.mak clean

key: logfile key.mak
	$(MAKE) -j -f key.mak clean all
	( ./key >> ${LOGFILE} )
	$(MAKE) -j -f key.mak clean

lighting: logfile lighting.mak
	$(MAKE) -j -f lighting.mak clean all
	( ./lighting >> ${LOGFILE} )
	$(MAKE) -j -f lighting.mak clean

lso: logfile lso.mak
	$(MAKE) -j -f lso.mak clean all
	( ./lso >> ${LOGFILE} )
	$(MAKE) -j -f lso.mak clean

memcopy: logfile memcopy.mak
	$(MAKE) -j -f memcopy.mak clean all
	( ./memcopy >> ${LOGFILE} )
	$(MAKE) -j -f memcopy.mak clean

npdu: logfile npdu.mak
	$(MAKE) -j -f npdu.mak clean all
	( ./npdu >> ${LOGFILE} )
	$(MAKE) -j -f npdu.mak clean

proplist: logfile proplist.mak
	$(MAKE) -j -f proplist.mak clean all
	( ./proplist >> ${LOGFILE} )
	$(MAKE) -j -f proplist.mak clean

ptransfer: logfile ptransfer.mak
	$(MAKE) -j -f ptransfer.mak clean all
	( ./ptransfer >> ${LOGFILE} )
	$(MAKE) -j -f ptransfer.mak clean

rd: logfile rd.mak
	$(MAKE) -j -f rd.mak clean all
	( ./rd >> ${LOGFILE} )
	$(MAKE) -j -f rd.mak clean

reject: logfile reject.mak
	$(MAKE) -j -f reject.mak clean all
	( ./reject >> ${LOGFILE} )
	$(MAKE) -j -f reject.mak clean

ringbuf: logfile ringbuf.mak
	$(MAKE) -j -f ringbuf.mak clean all
	( ./ringbuf >> ${LOGFILE} )
	$(MAKE) -j -f ringbuf.mak clean

rp: logfile rp.mak
	$(MAKE) -j -f rp.mak clean all
	( ./rp >> ${LOGFILE} )
	$(MAKE) -j -f rp.mak clean

rpm: logfile rpm.mak
	$(MAKE) -j -f rpm.mak clean all
	( ./rpm >> ${LOGFILE} )
	$(MAKE) -j -f rpm.mak clean

sbuf: logfile sbuf.mak
	$(MAKE) -j -f sbuf.mak clean all
	( ./sbuf >> ${LOGFILE} )
	$(MAKE) -j -f sbuf.mak clean

timesync: logfile timesync.mak
	$(MAKE) -j -f timesync.mak clean all
	( ./timesync >> ${LOGFILE} )
	$(MAKE) -j -f timesync.mak clean

vmac: logfile vmac.mak
	$(MAKE) -j -f vmac.mak clean all
	( ./vmac >> ${LOGFILE} )
	$(MAKE) -j -f vmac.mak clean

whohas: logfile whohas.mak
	$(MAKE) -j -f whohas.mak clean all
	( ./whohas >> ${LOGFILE} )
	$(MAKE) -j -f whohas.mak clean

whois: logfile whois.mak
	$(MAKE) -j -f whois.mak clean all
	( ./whois >> ${LOGFILE} )
	$(MAKE) -j -f whois.mak clean

wp: logfile wp.mak
	$(MAKE) -j -f wp.mak clean all
	( ./wp >> ${LOGFILE} )
	$(MAKE) -j -f wp.mak clean
