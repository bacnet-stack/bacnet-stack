#Makefile to build test case
CC      = gcc
BASEDIR = .
# -g for debugging with gdb
DEFINES = -DBIG_ENDIAN=0 -DTEST_RS485 -DBACDL_TEST
INCLUDES = -I. -I../../ 
CFLAGS  = -Wall $(INCLUDES) $(DEFINES) -g

SRCS = rs485.c

OBJS = ${SRCS:.c=.o}

TARGET = rs485

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
