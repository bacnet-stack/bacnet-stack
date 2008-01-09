all: library readprop writeprop readfile writefile reinit server dcc \
	whohas whois ucov timesync epics
	@echo "utilities are in the utils directory"

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
	demo/epics/Makefile
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

library: lib/Makefile
	( cd lib ; make )

readprop: demo/readprop/Makefile
	( cd demo/readprop ; make ; cp bacrp ../../utils )

writeprop: demo/writeprop/Makefile
	( cd demo/writeprop ; make ; cp bacwp ../../utils )

readfile: demo/readfile/Makefile
	( cd demo/readfile ; make ; cp bacarf ../../utils )

writefile: demo/writefile/Makefile
	( cd demo/writefile ; make ; cp bacawf ../../utils )

reinit: demo/reinit/Makefile
	( cd demo/reinit ; make ; cp bacrd ../../utils )

server: demo/server/Makefile
	( cd demo/server ; make ; cp bacserv ../../utils )

dcc: demo/dcc/Makefile
	( cd demo/dcc ; make ; cp bacdcc ../../utils )

whohas: demo/whohas/Makefile
	( cd demo/whohas ; make ; cp bacwh ../../utils )

timesync: demo/timesync/Makefile
	( cd demo/timesync ; make ; cp bacts ../../utils )

epics: demo/epics/Makefile
	( cd demo/epics ; make ; cp bacepics ../../utils )

ucov: demo/ucov/Makefile
	( cd demo/ucov ; make ; cp bacucov ../../utils )

whois: demo/whois/Makefile
	( cd demo/whois ; make ; cp bacwi ../../utils )

