all: readprop writeprop readfile writefile reinit server
	@echo "utilities are in demo/xx directories"

clean: demo/readprop/Makefile demo/writeprop/Makefile demo/readfile/Makefile demo/writefile/Makefile demo/server/Makefile
	( cd demo/readprop ; make clean )
	( cd demo/writeprop ; make clean )
	( cd demo/readfile ; make clean )
	( cd demo/writefile ; make clean )
	( cd demo/reinit ; make clean )
	( cd demo/server ; make clean )

readprop: demo/readprop/Makefile
	( cd demo/readprop ; make clean ; make )

writeprop: demo/writeprop/Makefile
	( cd demo/writeprop ; make clean ; make )

readfile: demo/readfile/Makefile
	( cd demo/readfile ; make clean ; make )

writefile: demo/writefile/Makefile
	( cd demo/writefile ; make clean ; make )

reinit: demo/reinit/Makefile
	( cd demo/reinit ; make clean ; make )

server: demo/server/Makefile
	( cd demo/server ; make clean ; make )

