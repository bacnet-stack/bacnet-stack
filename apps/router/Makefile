#Makefile to build BACnet Application for the Linux Port
# Note: requires libconfig-dev to be installed
#       sudo apt-get install -qq libconfig-dev

# tools - only if you need them.
# Most platforms have this already defined
# CC = gcc

# Executable file name
TARGET = router

TARGET_BIN = ${TARGET}$(TARGET_EXT)

ifeq (${BACNET_PORT},linux)
TARGET_EXT =
LIBS = -lpthread -lconfig -lm
LFLAGS = $(LIBS)
endif

SOURCE_DIR = ../../src
BACNET_SOURCE_DIR = ${SOURCE_DIR}/bacnet

SRCS = main.c \
	${BACNET_PORT_DIR}/rs485.c \
	${BACNET_PORT_DIR}/mstimer-init.c \
	${BACNET_PORT_DIR}/bip-init.c \
	${BACNET_PORT_DIR}/dlmstp_linux.c \
	${BACNET_SOURCE_DIR}/basic/bbmd/h_bbmd.c \
	${BACNET_SOURCE_DIR}/datalink/bvlc.c \
	${BACNET_SOURCE_DIR}/basic/sys/fifo.c \
	${BACNET_SOURCE_DIR}/datalink/cobs.c \
	${BACNET_SOURCE_DIR}/datalink/mstp.c \
	${BACNET_SOURCE_DIR}/datalink/mstptext.c \
	${BACNET_SOURCE_DIR}/basic/sys/debug.c \
	${BACNET_SOURCE_DIR}/indtext.c \
	${BACNET_SOURCE_DIR}/basic/sys/ringbuf.c \
	${BACNET_SOURCE_DIR}/datalink/crc.c \
	${BACNET_SOURCE_DIR}/bacdcode.c \
	${BACNET_SOURCE_DIR}/bacint.c \
	${BACNET_SOURCE_DIR}/bacreal.c \
	${BACNET_SOURCE_DIR}/bacstr.c \
	${BACNET_SOURCE_DIR}/npdu.c \
	${BACNET_SOURCE_DIR}/bacaddr.c \
	${BACNET_SOURCE_DIR}/hostnport.c \
	mstpmodule.c \
	ipmodule.c \
	portthread.c \
	msgqueue.c \
	network_layer.c

# note: router does not use common libbacnet.a library, 
# so use CFLAGS without common app defines or includes
CFLAGS = -I${SOURCE_DIR} -I${BACNET_PORT_DIR}
CFLAGS += -DBACNET_STACK_DEPRECATED_DISABLE
CFLAGS += -std=gnu99
CFLAGS += $(WARNINGS) $(DEBUGGING) $(OPTIMIZATION)

OBJS = ${SRCS:.c=.o}

all: Makefile ${TARGET_BIN}

${TARGET_BIN}: ${OBJS} Makefile
	${CC} ${OBJS} ${LFLAGS} -o $@
	size $@
	cp $@ ../../bin

.c.o:
	${CC} -c ${CFLAGS} $*.c -o $@

depend:
	rm -f .depend
	${CC} -MM ${CFLAGS} *.c >> .depend

clean:
	rm -f core ${TARGET_BIN} ${OBJS} $(TARGET).map

include: .depend
