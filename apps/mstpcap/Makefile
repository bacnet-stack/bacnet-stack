#Makefile to build BACnet Application

# Executable file name
TARGET = mstpcap

# BACNET_PORT, BACNET_PORT_DIR, BACNET_PORT_SRC are defined in common Makefile
# BACNET_SRC_DIR is defined in common apps Makefile
SRCS = main.c \
	${BACNET_PORT_DIR}/rs485.c \
	${BACNET_PORT_DIR}/mstimer-init.c \
	${BACNET_PORT_DIR}/datetime-init.c \
	${BACNET_SRC_DIR}/bacnet/bacdcode.c \
	${BACNET_SRC_DIR}/bacnet/bacint.c \
	${BACNET_SRC_DIR}/bacnet/bacreal.c \
	${BACNET_SRC_DIR}/bacnet/bacstr.c \
	${BACNET_SRC_DIR}/bacnet/iam.c \
	${BACNET_SRC_DIR}/bacnet/datetime.c \
	${BACNET_SRC_DIR}/bacnet/indtext.c \
	${BACNET_SRC_DIR}/bacnet/npdu.c \
	${BACNET_SRC_DIR}/bacnet/basic/sys/days.c \
	${BACNET_SRC_DIR}/bacnet/basic/sys/debug.c \
	${BACNET_SRC_DIR}/bacnet/basic/sys/fifo.c \
	${BACNET_SRC_DIR}/bacnet/basic/sys/filename.c \
	${BACNET_SRC_DIR}/bacnet/basic/sys/mstimer.c \
	${BACNET_SRC_DIR}/bacnet/basic/sys/ringbuf.c \
	${BACNET_SRC_DIR}/bacnet/datalink/cobs.c \
	${BACNET_SRC_DIR}/bacnet/datalink/mstp.c \
	${BACNET_SRC_DIR}/bacnet/datalink/mstptext.c \
	${BACNET_SRC_DIR}/bacnet/datalink/crc.c

# This demo seems to be a little unique
DEFINES = $(BACNET_DEFINES) -DBACDL_MSTP

# WARNINGS, DEBUGGING, OPTIMIZATION are defined in common apps Makefile
# BACNET_DEFINES is defined in common apps Makefile
# put all the flags together
INCLUDES = -I$(BACNET_SRC_DIR) -I$(BACNET_PORT_DIR)
CFLAGS += $(WARNINGS) $(DEBUGGING) $(OPTIMIZATION) $(BACNET_DEFINES) $(INCLUDES)
LFLAGS += -Wl,$(SYSTEM_LIB)
ifneq (${BACNET_LIB},)
LFLAGS += -Wl,$(BACNET_LIB)
endif
# GCC dead code removal
CFLAGS += -ffunction-sections -fdata-sections
ifeq ($(shell uname -s),Darwin)
LFLAGS += -Wl,-dead_strip
else
LFLAGS += -Wl,--gc-sections
endif
# GCC dead code removal

# TARGET_EXT defined in common apps Makefile
TARGET_BIN = ${TARGET}$(TARGET_EXT)

OBJS += ${SRCS:.c=.o}

.PHONY: all
all: Makefile ${TARGET_BIN}

${TARGET_BIN}: ${OBJS} Makefile
	${CC} ${PFLAGS} ${OBJS} ${LFLAGS} -o $@
	size $@
	cp $@ ../../bin
	
.c.o:
	${CC} -c ${CFLAGS} $*.c -o $@

.PHONY: depend
depend:
	rm -f .depend
	${CC} -MM ${CFLAGS} *.c >> .depend

.PHONY: clean
clean:
	rm -f core ${TARGET_BIN} ${OBJS} $(TARGET).map

.PHONY: include
include: .depend
