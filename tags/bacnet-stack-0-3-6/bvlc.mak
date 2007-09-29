#Makefile to build test case
CC      = gcc
BASEDIR = .
#CFLAGS  = -Wall -I.
# -g for debugging with gdb
#CFLAGS  = -Wall -I. -g
DEFINES = -DBACDL_BIP -DBIG_ENDIAN=0 -DTEST -DTEST_BVLC
INCLUDES = -Iports/linux -Itest -I.
CFLAGS  = -Wall $(INCLUDES)  $(DEFINES) -g

SRCS = bacdcode.c \
       bacint.c \
       bacstr.c \
       bigend.c \
       bvlc.c \
       test/ctest.c

OBJS = ${SRCS:.c=.o}

TARGET = bvlc

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
