all: library readprop writeprop readfile writefile reinit server dcc \
	whohas whois ucov timesync epics readpropm mstpcap \
	whoisrouter iamrouter initrouter 
	@echo "utilities are in the bin directory"

clean: lib/Makefile\
	demo/readprop/Makefile \
	demo/readpropm/Makefile \
	demo/writeprop/Makefile \
	demo/readfile/Makefile \
	demo/writefile/Makefile \
	demo/reinit/Makefile \
	demo/server/Makefile \
	demo/dcc/Makefile \
	demo/whohas/Makefile \
	demo/whois/Makefile \
	demo/ucov/Makefile \
	demo/timesync/Makefile \
	demo/epics/Makefile \
	demo/whoisrouter/Makefile \
	demo/iamrouter/Makefile \
	demo/initrouter/Makefile \
	demo/mstpcap/Makefile
	make -C lib clean
	make -C demo/readprop clean
	make -C demo/readpropm clean
	make -C demo/writeprop clean
	make -C demo/readfile clean
	make -C demo/writefile clean
	make -C demo/reinit clean
	make -C demo/server clean
	make -C demo/dcc clean
	make -C demo/whohas clean
	make -C demo/whois clean
	make -C demo/ucov clean
	make -C demo/timesync clean
	make -C demo/epics clean
	make -C demo/whoisrouter clean
	make -C demo/iamrouter clean
	make -C demo/initrouter clean
	make -C demo/mstpcap clean

library: lib/Makefile
	make -C lib all

readprop: demo/readprop/Makefile
	( cd demo/readprop ; make ; cp bacrp ../../bin )

readpropm: demo/readpropm/Makefile
	( cd demo/readpropm ; make ; cp bacrpm ../../bin )

writeprop: demo/writeprop/Makefile
	( cd demo/writeprop ; make ; cp bacwp ../../bin )

readfile: demo/readfile/Makefile
	( cd demo/readfile ; make ; cp bacarf ../../bin )

writefile: demo/writefile/Makefile
	( cd demo/writefile ; make ; cp bacawf ../../bin )

reinit: demo/reinit/Makefile
	( cd demo/reinit ; make ; cp bacrd ../../bin )

server: demo/server/Makefile
	( cd demo/server ; make ; cp bacserv ../../bin )

dcc: demo/dcc/Makefile
	( cd demo/dcc ; make ; cp bacdcc ../../bin )

whohas: demo/whohas/Makefile
	( cd demo/whohas ; make ; cp bacwh ../../bin )

timesync: demo/timesync/Makefile
	( cd demo/timesync ; make ; cp bacts ../../bin )

epics: demo/epics/Makefile
	( cd demo/epics ; make ; cp bacepics ../../bin )

ucov: demo/ucov/Makefile
	( cd demo/ucov ; make ; cp bacucov ../../bin )

whois: demo/whois/Makefile
	( cd demo/whois ; make ; cp bacwi ../../bin )

mstpcap: demo/mstpcap/Makefile
	( cd demo/mstpcap ; make clean all; cp mstpcap ../../bin )

whoisrouter: demo/whoisrouter/Makefile
	( cd demo/whoisrouter ; make ; cp bacwir ../../bin )

iamrouter: demo/iamrouter/Makefile
	( cd demo/iamrouter ; make ; cp baciamr ../../bin )

initrouter: demo/initrouter/Makefile
	( cd demo/initrouter ; make ; cp bacinitr ../../bin )

ports:	atmega168 at91sam7s bdk-atxx4-mstp
	echo "Built the ports"

atmega168: ports/atmega168/Makefile
	make -C ports/atmega168 clean all

at91sam7s: ports/at91sam7s/makefile
	make -C ports/at91sam7s clean all

bdk-atxx4-mstp: ports/bdk-atxx4-mstp/Makefile
	make -C ports/bdk-atxx4-mstp clean all




