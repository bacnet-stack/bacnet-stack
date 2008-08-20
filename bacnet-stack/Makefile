all: library readprop writeprop readfile writefile reinit server dcc \
	whohas whois ucov timesync epics mstpcap \
	whoisrouter iamrouter
	@echo "utilities are in the bin directory"

clean: lib/Makefile\
	demo/readprop/Makefile \
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
	demo/mstpcap/Makefile
	( cd lib ; make clean )
	( cd demo/readprop ; make clean )
	( cd demo/writeprop ; make clean )
	( cd demo/readfile ; make clean )
	( cd demo/writefile ; make clean )
	( cd demo/reinit ; make clean )
	( cd demo/server ; make clean )
	( cd demo/dcc ; make clean )
	( cd demo/whohas ; make clean )
	( cd demo/whois ; make clean )
	( cd demo/ucov ; make clean )
	( cd demo/timesync ; make clean )
	( cd demo/epics ; make clean )
	( cd demo/whoisrouter ; make clean )
	( cd demo/mstpcap ; make clean )

library: lib/Makefile
	( cd lib ; make )

readprop: demo/readprop/Makefile
	( cd demo/readprop ; make ; cp bacrp ../../bin )

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

