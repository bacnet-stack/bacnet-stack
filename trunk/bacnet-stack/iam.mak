#Makefile to build test case
CC      = gcc
BASEDIR = .
# -g for debugging with gdb
DEFINES = -DBACFILE=1 -DTEST -DBIG_ENDIAN=0 -DTEST_IAM -DBACDL_TEST
INCLUDES = -I. -Idemo/object -Itest 
CFLAGS  = -Wall $(INCLUDES) $(DEFINES) -g

SRCS = bacdcode.c \
       bacint.c \
       bacstr.c \
       bigend.c \
       npdu.c \
       apdu.c \
       dcc.c \
       iam.c \
       test/ctest.c

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
