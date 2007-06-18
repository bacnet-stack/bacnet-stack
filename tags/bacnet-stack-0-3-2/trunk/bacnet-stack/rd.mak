#Makefile to build test case
CC      = gcc
BASEDIR = .
#CFLAGS  = -Wall -I.
# -g for debugging with gdb
#CFLAGS  = -Wall -I. -g
CFLAGS  = -Wall -I. -Itest -DBIG_ENDIAN=0 -DTEST -DTEST_REINITIALIZE_DEVICE -g

SRCS = bacdcode.c \
       bacstr.c \
       bigend.c \
       rd.c \
       test/ctest.c

OBJS = ${SRCS:.c=.o}

TARGET = reinitialize_device

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
