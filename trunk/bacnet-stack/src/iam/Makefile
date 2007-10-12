#Makefile to build test case
CC      = gcc
BASEDIR = .
# -g for debugging with gdb
DEFINES = -DBACFILE=1 -DTEST -DBIG_ENDIAN=0 -DTEST_IAM -DBACDL_TEST
INCLUDES = -I. -I../../include -I../../test 
CFLAGS  = -Wall $(INCLUDES) $(DEFINES) -g

SRCS = ../bacdcode.c \
       ../bacint.c \
       ../bacstr.c \
       ../npdu.c \
       ../apdu.c \
       ../dcc.c \
       ../iam_client.c \
       ../iam_server.c \
       ../iam_test.c \
       ../../test/ctest.c

OBJS = ${SRCS:.c=.o}

TARGET = iam

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
