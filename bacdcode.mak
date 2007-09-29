#Makefile to build unit tests
CC      = gcc
BASEDIR = .
CFLAGS  = -Wall -I. -Itest -g -DBIG_ENDIAN=0 -DTEST -DTEST_DECODE

TARGET = bacdcode

SRCS = bacdcode.c \
       bacint.c \
       bacstr.c \
       test/ctest.c

OBJS = ${SRCS:.c=.o}

all: ${TARGET}
 
${TARGET}: ${OBJS}
	${CC} -o $@ ${OBJS} 

.c.o:
	${CC} -c ${CFLAGS} $*.c -o $@
	
depend:
	rm -f .depend
	${CC} -MM ${CFLAGS} *.c >> .depend
	
clean:
	rm -rf core ${OBJS} ${TARGET} *.bak

include: .depend
