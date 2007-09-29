#Makefile to build BACnet Application for the Linux Port

# Compiler to use
CC = gcc
# Executable file name
TARGET = bacserv

# Configure the BACnet Datalink Layer
#BACDL_DEFINE = -DBACDL_ETHERNET
#BACDL_DEFINE = -DBACDL_ARCNET
#BACDL_DEFINE = -DBACDL_MSTP
BACDL_DEFINE = -DBACDL_BIP -DBIP_DEBUG
BACNET_DEFINES = -DPRINT_ENABLED=1
DEFINES = $(BACNET_DEFINES) $(BACDL_DEFINE)

# Directories
BACNET_PORT = ../../ports/linux
BACNET_OBJECT = ../object
BACNET_HANDLER = ../handler
BACNET_ROOT = ../..
# BACnet Library
BACNET_LIB_DIR = ../../lib
BACNET_LIB_NAME = bacnet
BACNET_LIB_TARGET = $(BACNET_LIB_DIR)/lib$(BACNET_LIB_NAME).a
# Compiler Setup
INCLUDES = -I$(BACNET_ROOT) -I$(BACNET_PORT) -I$(BACNET_OBJECT) -I$(BACNET_HANDLER)
LIBRARIES=-lc,-lgcc,-lm,-L=$(BACNET_LIB_DIR),-l$(BACNET_LIB_NAME)
CFLAGS = -Wall -g $(INCLUDES) $(DEFINES)
LFLAGS = -Wl,-Map=$(TARGET).map,$(LIBRARIES)

SRCS = main.c

OBJS = ${SRCS:.c=.o}

all: ${BACNET_LIB_TARGET} Makefile ${TARGET}
	size ${TARGET}

${TARGET}: ${OBJS} Makefile
	${CC} -pthread  ${OBJS} ${LFLAGS} -o $@

lib: ${BACNET_LIB_TARGET}

${BACNET_LIB_TARGET}:
	( cd ${BACNET_LIB_DIR} ; make clean ; make )

.c.o:
	${CC} -c ${CFLAGS} $*.c -o $@

depend:
	rm -f .depend
	${CC} -MM ${CFLAGS} *.c >> .depend

clean:
	rm -f core ${TARGET} ${OBJS} ${BACNET_LIB_TARGET}

include: .depend
