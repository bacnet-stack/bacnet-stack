#Makefile to build test case
CC      = gcc
BASEDIR = .
#CFLAGS  = -Wall -I.
# -g for debugging with gdb
DEFINES  = -DBIG_ENDIAN=0 -DTEST -DTEST_BACNET_APPLICATION_DATA
CFLAGS  = -Wall -Iinclude -Itest -g $(DEFINES)

SRCS = src/bacdcode.c \
       src/bacint.c \
       src/bacstr.c \
       src/bacapp.c \
       src/datetime.c \
       src/bactext.c \
       src/indtext.c \
       test/ctest.c

OBJS = ${SRCS:.c=.o}

TARGET = bacapp

all: ${TARGET}
 
${TARGET}: ${OBJS}
	${CC} -o $@ ${OBJS} 

.c.o:
	${CC} -c ${CFLAGS} $*.c -o $@
	
depend:
	rm -f .depend
	${CC} -MM ${CFLAGS} *.c >> .depend
	
clean:
	rm -rf ${TARGET} $(OBJS) 

include: .depend
