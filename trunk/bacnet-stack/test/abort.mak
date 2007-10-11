#Makefile to build test case
CC      = gcc
BASEDIR = .
#CFLAGS  = -Wall -I.
# -g for debugging with gdb
#CFLAGS  = -Wall -I. -g
CFLAGS  = -Wall -Iinclude -Itest -DBIG_ENDIAN=0 -DTEST -DTEST_ABORT -g

SRC_DIR = src

SRCS = $(SRC_DIR)/bacdcode.c \
       $(SRC_DIR)/bacint.c \
       $(SRC_DIR)/bacstr.c \
       $(SRC_DIR)/abort.c \
       test/ctest.c

TARGET = abort

OBJS = ${SRCS:.c=.o}

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
