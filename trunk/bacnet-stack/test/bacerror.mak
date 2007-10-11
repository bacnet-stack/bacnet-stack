#Makefile to build test case
CC      = gcc
BASEDIR = .
#CFLAGS  = -Wall -I.
# -g for debugging with gdb
#CFLAGS  = -Wall -I. -g
DEFINES = -DBIG_ENDIAN=0 -DTEST -DTEST_BACERROR 
CFLAGS  = -Wall -Iinclude -Itest -g $(DEFINES)

SRCS = src/bacdcode.c \
       src/bacint.c \
       src/bacstr.c \
       src/bacerror.c \
       test/ctest.c

OBJS = ${SRCS:.c=.o}

TARGET = bacerror

all: ${TARGET}
 
${TARGET}: ${OBJS}
	${CC} -o $@ ${OBJS} 

.c.o:
	${CC} -c ${CFLAGS} $*.c -o $@
	
depend:
	rm -f .depend
	${CC} -MM ${CFLAGS} *.c >> .depend
	
clean:
	rm -rf ${TARGET} $(OBJS) 

include: .depend
