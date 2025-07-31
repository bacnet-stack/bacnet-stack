#Makefile to build test case
BASEDIR = .
BACNET_SRC_DIR ?= $(realpath ../../src)
# -g for debugging with gdb
DEFINES = -DBIG_ENDIAN=0 -DBACDL_MSTP=1 -DTEST_DLMSTP
DEFINES += -DBACNET_STACK_DEPRECATED_DISABLE
INCLUDES = -I. -I$(BACNET_SRC_DIR)
CFLAGS  = -Wall $(INCLUDES) $(DEFINES) -g

SRCS = rs485.c \
	dlmstp.c \
	mstimer-init.c \
	$(BACNET_SRC_DIR)/bacnet/datalink/cobs.c \
	$(BACNET_SRC_DIR)/bacnet/datalink/crc.c \
	$(BACNET_SRC_DIR)/bacnet/datalink/mstp.c \
	$(BACNET_SRC_DIR)/bacnet/datalink/mstptext.c \
	$(BACNET_SRC_DIR)/bacnet/basic/sys/debug.c \
	$(BACNET_SRC_DIR)/bacnet/basic/sys/fifo.c \
	$(BACNET_SRC_DIR)/bacnet/basic/sys/mstimer.c \
	$(BACNET_SRC_DIR)/bacnet/basic/sys/ringbuf.c \
	$(BACNET_SRC_DIR)/bacnet/bacaddr.c \
	$(BACNET_SRC_DIR)/bacnet/bacdcode.c \
	$(BACNET_SRC_DIR)/bacnet/bacint.c \
	$(BACNET_SRC_DIR)/bacnet/bacreal.c \
	$(BACNET_SRC_DIR)/bacnet/bacstr.c \
	$(BACNET_SRC_DIR)/bacnet/indtext.c \
	$(BACNET_SRC_DIR)/bacnet/npdu.c

OBJS = ${SRCS:.c=.o}

TARGET = dlmstp

all: ${TARGET}

${TARGET}: ${OBJS}
	${CC} -pthread -o $@ ${OBJS}
	size $@

.c.o:
	${CC} -c ${CFLAGS} $*.c -o $@

depend:
	rm -f .depend
	${CC} -MM ${CFLAGS} *.c >> .depend

clean:
	rm -rf core ${TARGET} $(OBJS) *.bak *.1 *.ini

include: .depend
