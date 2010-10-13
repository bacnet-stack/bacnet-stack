#Makefile to build BACnet Gateway Demonstration Application for the Linux Port

# tools - only if you need them.
# Most platforms have this already defined
# CC = gcc

# Executable file name
TARGET = bacgateway

# Configure the BACnet Datalink Layer
#BACDL_DEFINE = -DBACDL_ETHERNET
#BACDL_DEFINE = -DBACDL_ARCNET
#BACDL_DEFINE = -DBACDL_MSTP
BACDL_DEFINE = -DBACDL_BIP
BACNET_DEFINES = -DPRINT_ENABLED=1 -DBACAPP_ALL -DBACFILE
DEFINES = $(BACNET_DEFINES) $(BACDL_DEFINE)

# Directories
BACNET_PORT = linux
BACNET_PORT_DIR = ../../ports/${BACNET_PORT}
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
LIBRARIES=-lc,-lgcc,-lrt,-lm,-L$(BACNET_LIB_DIR),-l$(BACNET_LIB_NAME)
endif
ifeq (${BACNET_PORT},win32)
TARGET_BIN = ${TARGET}.exe
LIBRARY1=-L$(BACNET_LIB_DIR),-l$(BACNET_LIB_NAME)
LIBRARY2=-lws2_32,-lgcc,-lm,-liphlpapi,-lwinmm
LIBRARIES=$(LIBRARY1),$(LIBRARY2)
endif

#build for release (default) or debug
ifeq (${BUILD},debug)
# Use -g to put info for gdb in the executable
DEBUGGING = -g -DDEBUG_ENABLED=1
OPTIMIZATION = -O0
LINK_OPTIMIZATION = 
ifeq (${BACDL_DEFINE},-DBACDL_BIP=1)
DEFINES += -DBIP_DEBUG
endif
else 
DEBUGGING =
# Use -f optimizations and then link option --gc-sections to reduce executable size.
# Of course, not when you want debug output for gdb!
OPTIMIZATION = -Os -fdata-sections -ffunction-sections
LINK_OPTIMIZATION = -Wl,--gc-sections
endif

# put all the flags together
CFLAGS = -Wall $(DEBUGGING) $(OPTIMIZATION) $(INCLUDES) $(DEFINES) 
LFLAGS = -Wl,-Map=$(TARGET).map,$(LIBRARIES) $(LINK_OPTIMIZATION)

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
