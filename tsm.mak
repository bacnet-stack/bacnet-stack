#Makefile to build test case
CC      = gcc
# -g for debugging with gdb
DEFINES = -DTSM_ENABLED=1 -DTEST -DTEST_TSM -DBIG_ENDIAN=0 -DBACDL_TEST=1
INCLUDES = -I. -Idemo/object -Idemo/handler -Itest -Iports/linux
CFLAGS  = -Wall $(INCLUDES) $(DEFINES) -g

SRCS = address.c \
       bacdcode.c \
       bacint.c \
       bacstr.c \
       datetime.c \
       bacapp.c \
       bactext.c \
       indtext.c \
       demo/object/device.c \
       demo/object/ai.c \
       demo/object/ao.c \
       demo/object/av.c \
       demo/object/bi.c \
       demo/object/bo.c \
       demo/object/bv.c \
       demo/object/lsp.c \
       demo/object/mso.c \
       demo/object/lc.c \
       iam.c \
       dcc.c \
       npdu.c \
       apdu.c \
       tsm.c \
       version.c \
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
