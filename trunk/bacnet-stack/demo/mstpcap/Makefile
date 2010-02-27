#Makefile to build BACnet Application for the Linux Port

# Compiler to use
CC := gcc
# Executable file name
TARGET = mstpcap

# Configure the BACnet Datalink Layer
BACDL_DEFINE = -DBACDL_MSTP
BACNET_DEFINES = -DPRINT_ENABLED=1 -DBACAPP_ALL -DBACFILE
DEFINES = $(BACNET_DEFINES) $(BACDL_DEFINE)

# Directories
BACNET_PORT = linux
BACNET_PORT_DIR = ../../ports/${BACNET_PORT}
BACNET_SOURCE_DIR = ../../src
BACNET_INCLUDE = ../../include

# Compiler Setup
INCLUDES = -I$(BACNET_INCLUDE) -I$(BACNET_PORT_DIR)
ifeq (${BACNET_PORT},linux)
PFLAGS = -pthread
TARGET_BIN = ${TARGET}
LIBRARIES=-lc,-lgcc,-lrt,-lm
endif
ifeq (${BACNET_PORT},win32)
TARGET_BIN = ${TARGET}.exe
LIBRARIES=-lws2_32,-lgcc,-lm,-liphlpapi,-lwinmm
endif
#build for release (default) or debug
DEBUGGING =
OPTIMIZATION = -Os
ifeq (${BUILD},debug)
OPTIMIZATION = -O0
DEBUGGING = -g
endif
# put all the flags together
CFLAGS = -Wall $(DEBUGGING) $(OPTIMIZATION) $(INCLUDES) $(DEFINES) -fdata-sections -ffunction-sections
LFLAGS = -Wl,-Map=$(TARGET).map,$(LIBRARIES),--gc-sections

SRCS = main.c \
	${BACNET_PORT_DIR}/rs485.c \
	${BACNET_PORT_DIR}/timer.c \
	${BACNET_SOURCE_DIR}/mstp.c \
	${BACNET_SOURCE_DIR}/mstptext.c \
	${BACNET_SOURCE_DIR}/debug.c \
	${BACNET_SOURCE_DIR}/indtext.c \
	${BACNET_SOURCE_DIR}/ringbuf.c \
	${BACNET_SOURCE_DIR}/crc.c

OBJS = ${SRCS:.c=.o}

all: Makefile ${TARGET_BIN}
	size ${TARGET_BIN}

${TARGET_BIN}: ${OBJS} Makefile
	${CC} ${PFLAGS} ${OBJS} ${LFLAGS} -o $@

.c.o:
	${CC} -c ${CFLAGS} $*.c -o $@

depend:
	rm -f .depend
	${CC} -MM ${CFLAGS} *.c >> .depend

clean:
	rm -f core ${TARGET_BIN} ${OBJS} $(TARGET).map

include: .depend
