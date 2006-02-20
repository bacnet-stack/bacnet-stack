#Makefile to build test case
CC      = gcc
BASEDIR = .

DEFINES = -DBACFILE=1 -DBACDL_BIP=1 -DTEST -DTEST_ATOMIC_WRITE_FILE 
INCLUDES = -I. -Idemo/object -Itest 
CFLAGS  = -Wall $(INCLUDES) $(DEFINES) -g

SRCS = bacdcode.c \
       bacstr.c \
       bigend.c \
       awf.c \
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
	rm -rf core ${TARGET} $(OBJS) *.bak *.1 *.ini

include: .depend
