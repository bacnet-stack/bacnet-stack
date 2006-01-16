#Makefile to build test case
CC      = gcc
# -g for debugging with gdb
DEFINES = -DTEST -DTEST_TSM
INCLUDES = -I. -Idemo/object -Itest -Iports/linux
CFLAGS  = -Wall $(INCLUDES) $(DEFINES) -g

SRCS = address.c \
       bacdcode.c \
       bacstr.c \
       bigend.c \
       demo/object/device.c \
       demo/object/ai.c \
       demo/object/ao.c \
       iam.c \
       npdu.c \
       apdu.c \
       datalink.c \
       tsm.c \
       test/ctest.c

OBJS = ${SRCS:.c=.o}

TARGET = tsm

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
