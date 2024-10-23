#Makefile to build BACnet Application for the GCC port

# tools - only if you need them.
# Most platforms have this already defined
# CC = gcc

# Executable file name
TARGET = router-ipv6

TARGET_BIN = ${TARGET}$(TARGET_EXT)

# BACNET_PORT, BACNET_PORT_DIR, BACNET_PORT_SRC are defined in common Makefile
# BACNET_SRC_DIR is defined in common apps Makefile
BACNET_OBJECT_DIR = $(BACNET_SRC_DIR)/bacnet/basic/object
SRC = main.c \
	$(BACNET_SRC_DIR)/bacnet/basic/npdu/s_router.c \
	$(BACNET_OBJECT_DIR)/client/device-client.c \
	$(BACNET_OBJECT_DIR)/netport.c

# WARNINGS, DEBUGGING, OPTIMIZATION are defined in common apps Makefile
# BACNET_DEFINES is defined in common apps Makefile
# put all the flags together
INCLUDES = -I$(BACNET_SRC_DIR) -I$(BACNET_PORT_DIR)
SRCS = ${SRC}
OBJS += ${SRCS:.c=.o}

.PHONY: all
all: ${BACNET_LIB_TARGET} Makefile ${TARGET_BIN}

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
