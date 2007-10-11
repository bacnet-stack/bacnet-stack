#Makefile to build test case
CC      = gcc
BASEDIR = .
#CFLAGS  = -Wall -I.
# -g for debugging with gdb
#CFLAGS  = -Wall -I. -g
CFLAGS  = -Wall -I. -Itest -DBIG_ENDIAN=0 -DTEST -DTEST_READ_PROPERTY_MULTIPLE -g

SRCS = bacdcode.c \
       bacerror.c \
       bacapp.c \
       bactext.c \
       indtext.c \
       bacint.c \
       bacstr.c \
       datetime.c \
       rpm.c \
       test/ctest.c

OBJS = ${SRCS:.c=.o}

TARGET = rpm

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
