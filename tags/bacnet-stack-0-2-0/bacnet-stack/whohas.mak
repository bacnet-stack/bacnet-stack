#Makefile to build test case
CC      = gcc
BASEDIR = .
#CFLAGS  = -Wall -I.
# -g for debugging with gdb
#CFLAGS  = -Wall -I. -g
CFLAGS  = -Wall -I. -Itest -DTEST -DTEST_WHOHAS -g

SRCS = bacdcode.c \
       bacstr.c \
       bigend.c \
       whohas.c \
       test/ctest.c

OBJS = ${SRCS:.c=.o}

TARGET = whohas

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
