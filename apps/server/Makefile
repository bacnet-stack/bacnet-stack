#Makefile to build BACnet Application for the GCC port

# Executable file name
TARGET = bacserv
BACNET_OBJECT_DIR = $(BACNET_SRC_DIR)/bacnet/basic/object
SRC = main.c \
	$(BACNET_OBJECT_DIR)/device.c \
	$(BACNET_OBJECT_DIR)/ai.c \
	$(BACNET_OBJECT_DIR)/ao.c \
	$(BACNET_OBJECT_DIR)/av.c \
	$(BACNET_OBJECT_DIR)/bi.c \
	$(BACNET_OBJECT_DIR)/bo.c \
	$(BACNET_OBJECT_DIR)/bv.c \
	$(BACNET_OBJECT_DIR)/channel.c \
	$(BACNET_OBJECT_DIR)/command.c \
	$(BACNET_OBJECT_DIR)/csv.c \
	$(BACNET_OBJECT_DIR)/iv.c \
	$(BACNET_OBJECT_DIR)/lc.c \
	$(BACNET_OBJECT_DIR)/lo.c \
	$(BACNET_OBJECT_DIR)/lsp.c \
	$(BACNET_OBJECT_DIR)/ms-input.c \
	$(BACNET_OBJECT_DIR)/mso.c \
	$(BACNET_OBJECT_DIR)/msv.c \
	$(BACNET_OBJECT_DIR)/osv.c \
	$(BACNET_OBJECT_DIR)/piv.c \
	$(BACNET_OBJECT_DIR)/nc.c  \
	$(BACNET_OBJECT_DIR)/netport.c  \
	$(BACNET_OBJECT_DIR)/trendlog.c \
	$(BACNET_OBJECT_DIR)/schedule.c \
	$(BACNET_OBJECT_DIR)/access_credential.c \
	$(BACNET_OBJECT_DIR)/access_door.c \
	$(BACNET_OBJECT_DIR)/access_point.c \
	$(BACNET_OBJECT_DIR)/access_rights.c \
	$(BACNET_OBJECT_DIR)/access_user.c \
	$(BACNET_OBJECT_DIR)/access_zone.c \
	$(BACNET_OBJECT_DIR)/credential_data_input.c \
	$(BACNET_OBJECT_DIR)/acc.c \
	$(BACNET_OBJECT_DIR)/bacfile.c

BACNET_BASIC_SRC += \
	$(wildcard $(BACNET_SRC_DIR)/bacnet/basic/service/*.c)

# TARGET_EXT is defined in apps/Makefile as .exe or nothing
TARGET_BIN = ${TARGET}$(TARGET_EXT)

SRCS = $(SRC) $(BACNET_SRC) $(BACNET_BASIC_SRC) $(BACNET_PORT_SRC)

OBJS = ${SRCS:.c=.o}

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

