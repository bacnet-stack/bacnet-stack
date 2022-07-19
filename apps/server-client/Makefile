#Makefile to build BACnet Application using GCC compiler

# Executable file name - BACnet Server-Client Polling Application
TARGET = bacpoll
# BACnet objects that are used with this app
# BACnet objects that are used with this app
BACNET_OBJECT_DIR = $(BACNET_SRC_DIR)/bacnet/basic/object
BACNET_CLIENT_DIR = $(BACNET_SRC_DIR)/bacnet/basic/client
SRC = main.c \
	$(BACNET_OBJECT_DIR)/client/device-client.c \
	$(BACNET_OBJECT_DIR)/netport.c \
	$(BACNET_CLIENT_DIR)/bac-data.c \
	$(BACNET_CLIENT_DIR)/bac-rw.c \
	$(BACNET_CLIENT_DIR)/bac-task.c

# TARGET_EXT is defined in apps/Makefile as .exe or nothing
TARGET_BIN = ${TARGET}$(TARGET_EXT)

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

