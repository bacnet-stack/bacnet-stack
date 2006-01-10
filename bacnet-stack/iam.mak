#Makefile to build test case
CC      = gcc
BASEDIR = .
# -g for debugging with gdb
DEFINES = -DBACFILE=1 -DTEST -DTEST_IAM
INCLUDES = -I. -Idemo/object -Itest 
CFLAGS  = -Wall $(INCLUDES) $(DEFINES) -g

SRCS = bacdcode.c \
       bacstr.c \
       bigend.c \
       npdu.c \
       apdu.c \
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
