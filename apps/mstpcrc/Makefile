#Makefile to build BACnet Application

# Executable file name
TARGET = mstpcrc

SRCS = main.c \
	${BACNET_PORT_DIR}/mstimer-init.c \
	$(BACNET_SRC_DIR)/bacnet/basic/sys/mstimer.c \
	$(BACNET_SRC_DIR)/bacnet/datalink/crc.c

# BACNET_PORT, BACNET_PORT_DIR, BACNET_PORT_SRC are defined in common Makefile
# BACNET_SRC_DIR is defined in common apps Makefile
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

OBJS += ${SRCS:.c=.o}

TARGET_BIN = ${TARGET}$(TARGET_EXT)

.PHONY: all
all: Makefile ${TARGET_BIN}

${TARGET_BIN}: ${OBJS}
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
