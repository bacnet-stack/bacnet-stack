#Makefile to build test case
CC      = gcc
BASEDIR = .
#CFLAGS  = -Wall -I.
# -g for debugging with gdb
#CFLAGS  = -Wall -I. -g
CFLAGS  = -Wall -I. -Itest -DBIG_ENDIAN=0 -DTEST -DTEST_BINARY_OUTPUT -g

# NOTE: this file is normally called by the unittest.sh from up directory
SRCS = bacdcode.c \
       bacint.c \
       bacstr.c \
       datetime.c \
       bacapp.c \
       bactext.c \
       indtext.c \
       demo/object/bo.c \
       test/ctest.c

OBJS = ${SRCS:.c=.o}

TARGET = binary_output

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
