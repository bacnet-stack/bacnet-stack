#Makefile to build test case
CC      = gcc
BASEDIR = .

DEFINES = -DBACFILE=1 -DBACDL_BIP=1 -DBIG_ENDIAN=0 -DTEST -DTEST_ATOMIC_WRITE_FILE 
INCLUDES = -Iinclude -Idemo/object -Itest 
CFLAGS  = -Wall $(INCLUDES) $(DEFINES) -g

SRCS = src/bacdcode.c \
       src/bacint.c \
       src/bacstr.c \
       src/awf.c \
       test/ctest.c

OBJS = ${SRCS:.c=.o}

TARGET = atomicwritefile

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
