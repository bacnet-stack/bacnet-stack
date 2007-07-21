#Makefile to build test case
CC      = gcc
BASEDIR = .
# -g for debugging with gdb
DEFINES = -DBACFILE=1 -DBACDL_BIP=1 -DBIG_ENDIAN=0 -DTEST -DTEST_ATOMIC_READ_FILE 
INCLUDES = -I. -Idemo/object -Itest 
CFLAGS  = -Wall $(INCLUDES) $(DEFINES) -g

SRCS = bacdcode.c \
       bacstr.c \
       bigend.c \
       arf.c \
       test/ctest.c

OBJS = ${SRCS:.c=.o}

TARGET = atomicreadfile

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
