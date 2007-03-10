#Makefile to build test case
CC      = gcc
BASEDIR = .
#CFLAGS  = -Wall -I.
# -g for debugging with gdb
#CFLAGS  = -Wall -I. -g
CFLAGS  = -Wall -I. -Itest -DTEST -DTEST_LIGHTING_OUTPUT -g

# NOTE: this file is normally called by the unittest.sh from up directory
SRCS = bacdcode.c \
       bacstr.c \
       datetime.c \
       bacapp.c \
       bactext.c \
       indtext.c \
       demo/object/lo.c \
       test/ctest.c

OBJS = ${SRCS:.c=.o}

TARGET = lighting_output

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
