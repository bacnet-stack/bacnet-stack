#Makefile to build test case
CC      = gcc
BASEDIR = .
#CFLAGS  = -Wall -I.
# -g for debugging with gdb
#CFLAGS  = -Wall -I. -g
CFLAGS  = -Wall -I. -Itest -DTEST -DTEST_BINARY_VALUE -g

# NOTE: this file is normally called by the unittest.sh from up directory
SRCS = bacdcode.c \
       bacstr.c \
       bigend.c \
       bacapp.c \
       bactext.c \
       indtext.c \
       demo/object/bv.c \
       test/ctest.c

OBJS = ${SRCS:.c=.o}

TARGET = binary_value

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
