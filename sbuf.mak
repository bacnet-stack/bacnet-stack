#Makefile to build ringbuf tests
CC      = gcc
BASEDIR = .
#CFLAGS  = -Wall -I.
# -g for debugging with gdb
#CFLAGS  = -Wall -I. -g
CFLAGS  = -Wall -I. -Itest -DTEST -DTEST_STATIC_BUFFER -g

SRCS = sbuf.c \
	test/ctest.c

OBJS = ${SRCS:.c=.o}

TARGET = sbuf

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

