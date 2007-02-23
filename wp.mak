#Makefile to build test case
CC      = gcc
BASEDIR = .
# -g for debugging with gdb
DEFINES = -DBACFILE=1 -DBACDL_BIP=1 -DTEST -DTEST_WRITE_PROPERTY 
INCLUDES = -I. -Idemo/object -Itest 
CFLAGS  = -Wall $(INCLUDES) $(DEFINES) -g

SRCS = bacdcode.c \
       bacstr.c \
       datetime.c \
       bacapp.c \
       bactext.c \
       indtext.c \
       wp.c \
       test/ctest.c

OBJS = ${SRCS:.c=.o}

TARGET = writeproperty

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
