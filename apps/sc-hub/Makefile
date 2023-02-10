#Makefile to build BACnet Application using GCC compiler

# Executable file name
TARGET = bacschub
# BACnet objects that are used with this app
BACNET_OBJECT_DIR = $(BACNET_SRC_DIR)/bacnet/basic/object
SRC = main.c \
	$(BACNET_OBJECT_DIR)/client/device-client.c \
	$(BACNET_OBJECT_DIR)/netport.c

CFLAGS += -DBACNET_SECURE_CONNECT_ROUTING_TABLE=1
CFLAGS += -DBSC_CONF_HUB_FUNCTIONS_NUM=1
CFLAGS += -DBSC_CONF_HUB_CONNECTORS_NUM=1

# TARGET_EXT is defined in apps/Makefile as .exe or nothing
TARGET_BIN = ${TARGET}$(TARGET_EXT)

# TRICK! Use .obj extension to be sure Make doesn't skip lib .o files
OBJS := ${SRC:.c=.obj}

all: ${BACNET_LIB_TARGET} Makefile ${TARGET_BIN}

${TARGET_BIN}: ${OBJS} Makefile ${BACNET_LIB_TARGET}
	${CC} ${PFLAGS} ${OBJS} ${LFLAGS} -o $@
	size $@
	cp $@ ../../bin

${BACNET_LIB_TARGET}:
	( cd ${BACNET_LIB_DIR} ; $(MAKE) clean ; $(MAKE) -s )

# TRICK! overridden object files use .obj extension
%.obj:%.c
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

