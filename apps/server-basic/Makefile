#Makefile to build BACnet Application using GCC compiler

# Executable file name
TARGET = bacbasic
# BACnet objects supporting CreateObject that are used with this app
BACNET_OBJECT_DIR = $(BACNET_SRC_DIR)/bacnet/basic/object
BACNET_SERVER_DIR = $(BACNET_SRC_DIR)/bacnet/basic/server
SRC = main.c \
	$(BACNET_SERVER_DIR)/bacnet_basic.c \
	$(BACNET_SERVER_DIR)/bacnet_device.c \
	$(BACNET_SERVER_DIR)/bacnet_port.c \
	$(BACNET_SERVER_DIR)/bacnet_port_ipv4.c \
	$(BACNET_SERVER_DIR)/bacnet_port_ipv6.c \
	$(BACNET_SERVER_DIR)/bacnet_port_mstp.c \
	$(BACNET_OBJECT_DIR)/ai.c \
	$(BACNET_OBJECT_DIR)/ao.c \
	$(BACNET_OBJECT_DIR)/av.c \
	$(BACNET_OBJECT_DIR)/auditlog.c \
	$(BACNET_OBJECT_DIR)/bacfile.c \
	$(BACNET_OBJECT_DIR)/bi.c \
	$(BACNET_OBJECT_DIR)/bitstring_value.c \
	$(BACNET_OBJECT_DIR)/blo.c \
	$(BACNET_OBJECT_DIR)/bo.c \
	$(BACNET_OBJECT_DIR)/bv.c \
	$(BACNET_OBJECT_DIR)/calendar.c \
	$(BACNET_OBJECT_DIR)/channel.c \
	$(BACNET_OBJECT_DIR)/color_object.c \
	$(BACNET_OBJECT_DIR)/color_temperature.c \
	$(BACNET_OBJECT_DIR)/command.c \
	$(BACNET_OBJECT_DIR)/csv.c \
	$(BACNET_OBJECT_DIR)/iv.c \
	$(BACNET_OBJECT_DIR)/lc.c \
	$(BACNET_OBJECT_DIR)/lo.c \
	$(BACNET_OBJECT_DIR)/loop.c \
	$(BACNET_OBJECT_DIR)/lsp.c \
	$(BACNET_OBJECT_DIR)/lsz.c \
	$(BACNET_OBJECT_DIR)/ms-input.c \
	$(BACNET_OBJECT_DIR)/mso.c \
	$(BACNET_OBJECT_DIR)/msv.c \
	$(BACNET_OBJECT_DIR)/nc.c  \
	$(BACNET_OBJECT_DIR)/netport.c  \
	$(BACNET_OBJECT_DIR)/osv.c \
	$(BACNET_OBJECT_DIR)/piv.c \
	$(BACNET_OBJECT_DIR)/program.c  \
	$(BACNET_OBJECT_DIR)/structured_view.c \
	$(BACNET_OBJECT_DIR)/timer.c  \
	$(BACNET_OBJECT_DIR)/time_value.c

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
