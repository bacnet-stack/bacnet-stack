#Makefile to build BACnet Application for the GCC port

# tools - only if you need them.
# Most platforms have this already defined
# CC = gcc

# Executable file name
TARGET = bacroute

TARGET_BIN = ${TARGET}$(TARGET_EXT)

CFLAGS += -DPRINT_ENABLED=1

BACNET_SOURCE_DIR = ../../src
BACNET_HANDLER_DIR = ../handler
BACNET_OBJECT_DIR = ../object

SRC = main.c \
	$(BACNET_OBJECT_DIR)/device-client.c

PORT_BIP6_SRC = \
	$(BACNET_PORT_DIR)/bip6.c \
	$(BACNET_SOURCE_DIR)/bvlc6.c \
	$(BACNET_HANDLER_DIR)/h_bbmd6.c \
	$(BACNET_SOURCE_DIR)/vmac.c

SRCS = ${SRC} ${PORT_BIP6_SRC}

OBJS = ${SRCS:.c=.o}

all: ${BACNET_LIB_TARGET} Makefile ${TARGET_BIN}

${TARGET_BIN}: ${OBJS} Makefile ${BACNET_LIB_TARGET}
	${CC} ${PFLAGS} ${OBJS} ${LFLAGS} -o $@
	size $@
	cp $@ ../../bin

lib: ${BACNET_LIB_TARGET}

${BACNET_LIB_TARGET}:
	( cd ${BACNET_LIB_DIR} ; $(MAKE) clean ; $(MAKE) )

.c.o:
	${CC} -c ${CFLAGS} $*.c -o $@

depend:
	rm -f .depend
	${CC} -MM ${CFLAGS} *.c >> .depend

clean:
	rm -f core ${TARGET_BIN} ${OBJS} ${BACNET_LIB_TARGET} $(TARGET).map

include: .depend
