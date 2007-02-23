#Makefile to build test case
CC      = gcc
BASEDIR = .
#CFLAGS  = -Wall -I.
# -g for debugging with gdb
#CFLAGS  = -Wall -I. -g
CFLAGS  = -Wall -I. -Itest -Idemo/object -DTEST -DTEST_COV -DBACDL_TEST=1 -DBIG_ENDIAN=0 -g

SRCS = bacdcode.c \
       bacstr.c \
       datetime.c \
       bacapp.c \
       indtext.c \
       bactext.c \
       cov.c \
       test/ctest.c

OBJS = ${SRCS:.c=.o}

TARGET = cov

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
