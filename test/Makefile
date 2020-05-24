# Unit tests for the BACnet Stack project

LOGFILE = test.log

all: abort address arf awf bvlc bvlc6 bacapp bacdcode bacerror bacint bacstr \
	cov crc datetime dcc event filename fifo getevent iam ihave \
	indtext keylist key lighting lso memcopy npdu proplist ptransfer \
	rd reject ringbuf rp rpm sbuf timesync vmac \
	bbmd bbmd6 objects \
	whohas whois wp

clean: logfile
	rm ${LOGFILE}

logfile:
	touch ${LOGFILE}

report:
	cat ${LOGFILE}
	cat ${LOGFILE} | grep Failed: | \
	awk 'BEGIN { err = 0 } $$2 > 0 { print "Unit Tests Failed!"; err = 1} \
	END {exit err}'

abort: logfile abort.mak
	$(MAKE) -s -f abort.mak clean all
	( ./abort >> ${LOGFILE} )
	$(MAKE) -s -f abort.mak clean

address: logfile address.mak
	$(MAKE) -s -f address.mak clean all
	( ./address >> ${LOGFILE} )
	$(MAKE) -s -f address.mak clean

arf: logfile arf.mak
	$(MAKE) -s -f arf.mak clean all
	( ./arf >> ${LOGFILE} )
	$(MAKE) -s -f arf.mak clean

awf: logfile awf.mak
	$(MAKE) -s -f awf.mak clean all
	( ./awf >> ${LOGFILE} )
	$(MAKE) -s -f awf.mak clean

bacapp: logfile bacapp.mak
	$(MAKE) -s -f bacapp.mak clean all
	( ./bacapp >> ${LOGFILE} )
	$(MAKE) -s -f bacapp.mak clean

bacdcode: logfile bacdcode.mak
	$(MAKE) -s -f bacdcode.mak clean all
	( ./bacdcode >> ${LOGFILE} )
	$(MAKE) -s -f bacdcode.mak clean

bacerror: logfile bacerror.mak
	$(MAKE) -s -f bacerror.mak clean all
	( ./bacerror >> ${LOGFILE} )
	$(MAKE) -s -f bacerror.mak clean

bacint: logfile bacint.mak
	$(MAKE) -s -f bacint.mak clean all
	( ./bacint >> ${LOGFILE} )
	$(MAKE) -s -f bacint.mak clean

bacreal: logfile bacnet/bacreal/Makefile
	$(MAKE) -s -C bacnet/bacreal/ clean all
	( ./bacnet/bacreal/unittest >> ${LOGFILE} )
	$(MAKE) -s -C bacnet/bacreal/ clean

bacstr: logfile bacstr.mak
	$(MAKE) -s -f bacstr.mak clean all
	( ./bacstr >> ${LOGFILE} )
	$(MAKE) -s -f bacstr.mak clean

bvlc: logfile bvlc.mak
	$(MAKE) -s -f bvlc.mak clean all
	( ./bvlc >> ${LOGFILE} )
	$(MAKE) -s -f bvlc.mak clean

bbmd: logfile bacnet/basic/bbmd/Makefile
	$(MAKE) -s -C bacnet/basic/bbmd/ clean all
	( ./bacnet/basic/bbmd/unittest >> ${LOGFILE} )
	$(MAKE) -s -C bacnet/basic/bbmd/ clean

bvlc6: logfile bvlc6.mak
	$(MAKE) -s -f bvlc6.mak clean all
	( ./bvlc6 >> ${LOGFILE} )
	$(MAKE) -s -f bvlc6.mak clean

bbmd6: logfile bacnet/basic/bbmd6/Makefile
	$(MAKE) -s -C bacnet/basic/bbmd6/ clean all
	( ./bacnet/basic/bbmd6/unittest >> ${LOGFILE} )
	$(MAKE) -s -C bacnet/basic/bbmd6/ clean

cov: logfile cov.mak
	$(MAKE) -s -f cov.mak clean all
	( ./cov >> ${LOGFILE} )
	$(MAKE) -s -f cov.mak clean

crc: logfile crc.mak
	$(MAKE) -s -f crc.mak clean all
	( ./crc >> ${LOGFILE} )
	$(MAKE) -s -f crc.mak clean

datetime: logfile datetime.mak
	$(MAKE) -s -f datetime.mak clean all
	( ./datetime >> ${LOGFILE} )
	$(MAKE) -s -f datetime.mak clean

dcc: logfile dcc.mak
	$(MAKE) -s -f dcc.mak clean all
	( ./dcc >> ${LOGFILE} )
	$(MAKE) -s -f dcc.mak clean

event: logfile event.mak
	$(MAKE) -s -f event.mak clean all
	( ./event >> ${LOGFILE} )
	$(MAKE) -s -f event.mak clean

filename: logfile filename.mak
	$(MAKE) -s -f filename.mak clean all
	( ./filename >> ${LOGFILE} )
	$(MAKE) -s -f filename.mak clean

fifo: logfile fifo.mak
	$(MAKE) -s -f fifo.mak clean all
	( ./fifo >> ${LOGFILE} )
	$(MAKE) -s -f fifo.mak clean

getevent: logfile getevent.mak
	$(MAKE) -s -f getevent.mak clean all
	( ./getevent >> ${LOGFILE} )
	$(MAKE) -s -f getevent.mak clean

iam: logfile iam.mak
	$(MAKE) -s -f iam.mak clean all
	( ./iam >> ${LOGFILE} )
	$(MAKE) -s -f iam.mak clean

ihave: logfile ihave.mak
	$(MAKE) -s -f ihave.mak clean all
	( ./ihave >> ${LOGFILE} )
	$(MAKE) -s -f ihave.mak clean

indtext: logfile indtext.mak
	$(MAKE) -s -f indtext.mak clean all
	( ./indtext >> ${LOGFILE} )
	$(MAKE) -s -f indtext.mak clean

keylist: logfile keylist.mak
	$(MAKE) -s -f keylist.mak clean all
	( ./keylist >> ${LOGFILE} )
	$(MAKE) -s -f keylist.mak clean

key: logfile key.mak
	$(MAKE) -s -f key.mak clean all
	( ./key >> ${LOGFILE} )
	$(MAKE) -s -f key.mak clean

lighting: logfile lighting.mak
	$(MAKE) -s -f lighting.mak clean all
	( ./lighting >> ${LOGFILE} )
	$(MAKE) -s -f lighting.mak clean

lso: logfile lso.mak
	$(MAKE) -s -f lso.mak clean all
	( ./lso >> ${LOGFILE} )
	$(MAKE) -s -f lso.mak clean

memcopy: logfile memcopy.mak
	$(MAKE) -s -f memcopy.mak clean all
	( ./memcopy >> ${LOGFILE} )
	$(MAKE) -s -f memcopy.mak clean

npdu: logfile npdu.mak
	$(MAKE) -s -f npdu.mak clean all
	( ./npdu >> ${LOGFILE} )
	$(MAKE) -s -f npdu.mak clean

objects: logfile bacnet/basic/object/objects/Makefile
	$(MAKE) -s -C bacnet/basic/object/objects/ clean all
	( ./bacnet/basic/object/objects/unittest >> ${LOGFILE} )
	$(MAKE) -s -C bacnet/basic/object/objects/ clean

proplist: logfile proplist.mak
	$(MAKE) -s -f proplist.mak clean all
	( ./proplist >> ${LOGFILE} )
	$(MAKE) -s -f proplist.mak clean

ptransfer: logfile ptransfer.mak
	$(MAKE) -s -f ptransfer.mak clean all
	( ./ptransfer >> ${LOGFILE} )
	$(MAKE) -s -f ptransfer.mak clean

rd: logfile rd.mak
	$(MAKE) -s -f rd.mak clean all
	( ./rd >> ${LOGFILE} )
	$(MAKE) -s -f rd.mak clean

reject: logfile reject.mak
	$(MAKE) -s -f reject.mak clean all
	( ./reject >> ${LOGFILE} )
	$(MAKE) -s -f reject.mak clean

ringbuf: logfile ringbuf.mak
	$(MAKE) -s -f ringbuf.mak clean all
	( ./ringbuf >> ${LOGFILE} )
	$(MAKE) -s -f ringbuf.mak clean

rp: logfile rp.mak
	$(MAKE) -s -f rp.mak clean all
	( ./rp >> ${LOGFILE} )
	$(MAKE) -s -f rp.mak clean

rpm: logfile rpm.mak
	$(MAKE) -s -f rpm.mak clean all
	( ./rpm >> ${LOGFILE} )
	$(MAKE) -s -f rpm.mak clean

sbuf: logfile sbuf.mak
	$(MAKE) -s -f sbuf.mak clean all
	( ./sbuf >> ${LOGFILE} )
	$(MAKE) -s -f sbuf.mak clean

timesync: logfile timesync.mak
	$(MAKE) -s -f timesync.mak clean all
	( ./timesync >> ${LOGFILE} )
	$(MAKE) -s -f timesync.mak clean

vmac: logfile vmac.mak
	$(MAKE) -s -f vmac.mak clean all
	( ./vmac >> ${LOGFILE} )
	$(MAKE) -s -f vmac.mak clean

whohas: logfile whohas.mak
	$(MAKE) -s -f whohas.mak clean all
	( ./whohas >> ${LOGFILE} )
	$(MAKE) -s -f whohas.mak clean

whois: logfile whois.mak
	$(MAKE) -s -f whois.mak clean all
	( ./whois >> ${LOGFILE} )
	$(MAKE) -s -f whois.mak clean

wp: logfile wp.mak
	$(MAKE) -s -f wp.mak clean all
	( ./wp >> ${LOGFILE} )
	$(MAKE) -s -f wp.mak clean
