###############################################################################
# Makefile for Project - BACnet Blinkt!
# sudo apt install libpigpio-dev libpigpiod-if-dev pigpiod
###############################################################################

## General Flags
TARGET = bacblinkt
# BACnet objects that are used with this app
BACNET_OBJECT_DIR = $(BACNET_SRC_DIR)/bacnet/basic/object
SRC = main.c \
	blinkt.c \
	device.c \
	$(BACNET_OBJECT_DIR)/netport.c \
	$(BACNET_OBJECT_DIR)/channel.c \
	$(BACNET_OBJECT_DIR)/color_object.c \
	$(BACNET_OBJECT_DIR)/color_temperature.c \
	$(BACNET_OBJECT_DIR)/lc.c \
	$(BACNET_OBJECT_DIR)/lo.c

CFLAGS += -DMAX_TSM_TRANSACTIONS=1

# TARGET_EXT is defined in apps/Makefile as .exe or nothing
TARGET_BIN = ${TARGET}$(TARGET_EXT)

# note: link to pigpio daemon so that our app can run without root
# start pigio daemon with the command 'sudo pigpiod'
LD_PIGPIO = -Wl,-pthread,-lpigpiod_if2
LFLAGS += $(LD_PIGPIO)

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

