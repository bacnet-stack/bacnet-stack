#Makefile to build unit tests
CC      = gcc
BASEDIR = .
#CFLAGS  = -Wall -I.
# -g for debugging with gdb
CFLAGS  = -Wall -I. -Itest -g -DTEST -DTEST_KEY -DTEST_KEYLIST

KEY_SRCS = key.c \
       test/ctest.c

KEYLIST_SRCS = keylist.c \
       test/ctest.c

OBJS = ${SRCS:.c=.o}

KEY_OBJS  = ${KEY_SRCS:.c=.o}

KEYLIST_OBJS  = ${KEYLIST_SRCS:.c=.o}

all: key keylist
 
key: ${KEY_OBJS}
	${CC} -o $@ ${KEY_OBJS} 

keylist: ${KEYLIST_OBJS}
	${CC} -o $@ ${KEYLIST_OBJS} 

.c.o:
	${CC} -c ${CFLAGS} $*.c
	
depend:
	rm -f .depend
	${CC} -MM ${CFLAGS} *.c >> .depend
	
clean:
	rm -rf core ${TARGET} $(OBJS) *.bak *.1

include: .depend
