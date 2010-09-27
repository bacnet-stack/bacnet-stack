#Makefile to build BACnet Application for the Linux Port

# tools - only if you need them.
# Most platforms have this already defined
# CC = gcc

TARGET = bacepics

# Configure the BACnet Datalink Layer
#BACDL_DEFINE = -DBACDL_ETHERNET=1
#BACDL_DEFINE = -DBACDL_ARCNET=1
#BACDL_DEFINE = -DBACDL_MSTP=1
BACDL_DEFINE = -DBACDL_BIP=1
BACNET_DEFINES = -DBACFILE=1 -DPRINT_ENABLED=1 -DBACAPP_ALL
DEFINES = $(BACNET_DEFINES) $(BACDL_DEFINE)

# Directories
BACNET_PORT = linux
BACNET_PORT_DIR = ../../ports/${BACNET_PORT}
BACNET_SOURCE = ../../src
BACNET_INCLUDE = ../../include
# BACnet Library
BACNET_LIB_DIR = ../../lib
BACNET_LIB_NAME = bacnet
BACNET_LIB_TARGET = $(BACNET_LIB_DIR)/lib$(BACNET_LIB_NAME).a
# Compiler Setup
INCLUDES = -I$(BACNET_INCLUDE) -I$(BACNET_PORT_DIR)
ifeq (${BACNET_PORT},linux)
PFLAGS = -pthread
TARGET_BIN = ${TARGET}
LIBRARIES=-L$(BACNET_LIB_DIR),-l$(BACNET_LIB_NAME),-lc,-lgcc,-lm
endif
ifeq (${BACNET_PORT},win32)
TARGET_BIN = ${TARGET}.exe
LIBRARY1=-L$(BACNET_LIB_DIR),-l$(BACNET_LIB_NAME)
LIBRARY2=-lws2_32,-lgcc,-lm,-liphlpapi,-lwinmm
LIBRARIES=$(LIBRARY1),$(LIBRARY2)
endif
#build for release (default) or debug
DEBUGGING =
OPTIMIZATION = -Os
ifeq (${BUILD},debug)
OPTIMIZATION = -O0
DEBUGGING = -g -DDEBUG_ENABLED=1
endif
# put all the flags together
CFLAGS = -Wall $(DEBUGGING) $(OPTIMIZATION) $(INCLUDES) $(DEFINES)
LFLAGS = -Wl,-Map=$(TARGET).map,$(LIBRARIES)

SRCS = main.c

OBJS = ${SRCS:.c=.o}

all: ${BACNET_LIB_TARGET} Makefile ${TARGET_BIN}
	size ${TARGET_BIN}

${TARGET_BIN}: ${OBJS} Makefile ${BACNET_LIB_TARGET}
	${CC} ${PFLAGS} ${OBJS} ${LFLAGS} -o $@

lib: ${BACNET_LIB_TARGET}

${BACNET_LIB_TARGET}:
	( cd ${BACNET_LIB_DIR} ; make clean ; make )

.c.o:
	${CC} -c ${CFLAGS} $*.c -o $@

depend:
	rm -f .depend
	${CC} -MM ${CFLAGS} *.c >> .depend

clean:
	rm -f core ${TARGET_BIN} ${OBJS} ${BACNET_LIB_TARGET} $(TARGET).map

include: .depend
