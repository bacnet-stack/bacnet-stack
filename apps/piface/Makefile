###############################################################################
# Makefile for Project - PiFace
###############################################################################

## General Flags
TARGET = bacpiface
# BACnet objects that are used with this app
BACNET_OBJECT_DIR = $(BACNET_SRC_DIR)/bacnet/basic/object
SRC = main.c \
	device.c \
	$(BACNET_OBJECT_DIR)/netport.c \
	$(BACNET_OBJECT_DIR)/bi.c \
	$(BACNET_OBJECT_DIR)/bo.c

CFLAGS += -DMAX_TSM_TRANSACTIONS=1
CFLAGS += -DMAX_BINARY_INPUTS=4
CFLAGS += -DMAX_BINARY_OUTPUTS=8

# TARGET_EXT is defined in apps/Makefile as .exe or nothing
TARGET_BIN = ${TARGET}$(TARGET_EXT)

# Source locations
PIFACE_INCLUDE = libpifacedigital/src
PIFACE_LIB = libpifacedigital
MCP23S17_LIB = libmcp23s17

CFLAGS += -I$(PIFACE_INCLUDE)

LDPIFACE = -Wl,-L$(PIFACE_LIB),-lpifacedigital,-L$(MCP23S17_LIB),-lmcp23s17
LFLAGS += $(LDPIFACE)

OBJS += ${SRC:.c=.o}

all: ${BACNET_LIB_TARGET} Makefile ${TARGET_BIN}

${TARGET_BIN}: ${OBJS} Makefile ${BACNET_LIB_TARGET}
	${CC} ${PFLAGS} ${OBJS} ${LFLAGS} -o $@
	size $@
	cp $@ ../../bin

${BACNET_LIB_TARGET}:
	( cd ${BACNET_LIB_DIR} ; $(MAKE) clean ; $(MAKE) -s )

.c.o:
	${CC} -c ${CFLAGS} $*.c -o $@

.PHONY: depend
depend:
	rm -f .depend
	${CC} -MM ${CFLAGS} *.c >> .depend

.PHONY: clean
clean:
	rm -f core ${TARGET_BIN} ${OBJS} $(TARGET).map ${BACNET_LIB_TARGET}

.PHONY: include
include: .depend

