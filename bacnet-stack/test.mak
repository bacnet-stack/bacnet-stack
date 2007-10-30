LOGFILE = test.log

all: abort datetime

clean:
	rm ${LOGFILE}

logfile:
	touch ${LOGFILE}

abort: test/abort.mak logfile
	( cd test ; make -f abort.mak clean )
	( cd test ; make -f abort.mak )
	( ./test/abort >> ${LOGFILE} )
	( cd test ; make -f abort.mak clean )

datetime: test/datetime.mak logfile
	( cd test ; make -f datetime.mak clean )
	( cd test ; make -f datetime.mak )
	( ./test/datetime >> ${LOGFILE} )
	( cd test ; make -f datetime.mak clean )
