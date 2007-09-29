#Makefile to build test case
CC      = gcc
BASEDIR = .
#CFLAGS  = -Wall -I.
# -g for debugging with gdb
#CFLAGS  = -Wall -I. -g
CFLAGS  = -g -Wall -I. -Itest -DBIG_ENDIAN=0 -DTEST -DTEST_DEVICE -DBACDL_TEST -DMAX_TSM_TRANSACTIONS=0

# NOTE: this file is normally called by the unittest.sh from up directory
SRCS = bacdcode.c \
       datetime.c \
       bacint.c \
       bacstr.c \
       bacapp.c \
       bactext.c \
       indtext.c \
       apdu.c \
       dcc.c \
       version.c \
       demo/object/device.c \
       test/ctest.c

OBJS = ${SRCS:.c=.o}

TARGET = device

all: ${TARGET}
 
${TARGET}: ${OBJS}
	${CC} -o $@ ${OBJS} 

.c.o:
	${CC} -c ${CFLAGS} $*.c -o $@
	
depend:
	rm -f .depend
	${CC} -MM ${CFLAGS} *.c >> .depend
	
clean:
	rm -rf core ${TARGET} $(OBJS) *.bak *.1 *.ini

include: .depend
