#Makefile to build BACnet Application using GCC compiler

# Executable file name
TARGET = bacwebsocketssrv
# BACnet objects that are used with this app
#BACNET_OBJECT_DIR = $(BACNET_SRC_DIR)/datalink/bsc

SRC = main.c \
	$(BACNET_PORT_DIR)/websocket-srv.c \
	$(BACNET_PORT_DIR)/websocket-cli.c

# TARGET_EXT is defined in apps/Makefile as .exe or nothing
TARGET_BIN = ${TARGET}$(TARGET_EXT)

OBJS += ${SRC:.c=.o}

#all: ${BACNET_LIB_TARGET} Makefile ${TARGET_BIN}

all: Makefile ${TARGET_BIN}

#${TARGET_BIN}: ${OBJS} Makefile ${BACNET_LIB_TARGET}
#	${CC} ${PFLAGS} ${OBJS} -Wl,-L/usr/local/lib,-lwebsockets ${LFLAGS} -o $@

${TARGET_BIN}: ${OBJS} Makefile
	${CC} ${PFLAGS} ${OBJS} -I/usr/local/include /usr/local/lib/libwebsockets.a -lcap -lssl -lcrypto ${LFLAGS} -o $@
	size $@
	cp $@ ../../bin

#${BACNET_LIB_TARGET}:
#	( cd ${BACNET_LIB_DIR} ; $(MAKE) clean ; $(MAKE) -s )

.c.o:
	${CC} -c ${CFLAGS} $*.c -o $@

.PHONY: depend
depend:
	rm -f .depend
	${CC} -MM ${CFLAGS} *.c >> .depend

.PHONY: clean
clean:
	rm -f core ${TARGET_BIN} ${OBJS} $(TARGET).map

#	rm -f core ${TARGET_BIN} ${OBJS} $(TARGET).map ${BACNET_LIB_TARGET}

.PHONY: include
include: .depend

