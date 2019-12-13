#Makefile to build BACnet Application using GCC compiler

# Executable file name
TARGET = bacrp
# BACnet objects that are used with this app
BACNET_OBJECT_DIR = $(BACNET_SRC_DIR)/bacnet/basic/object
SRC = main.c \
	$(BACNET_OBJECT_DIR)/client/device-client.c \
	$(BACNET_OBJECT_DIR)/netport.c
BACNET_BASIC_SRC += \
	$(BACNET_SRC_DIR)/bacnet/basic/service/h_apdu.c \
	$(BACNET_SRC_DIR)/bacnet/basic/service/h_iam.c \
	$(BACNET_SRC_DIR)/bacnet/basic/service/h_noserv.c \
	$(BACNET_SRC_DIR)/bacnet/basic/service/h_rp.c \
	$(BACNET_SRC_DIR)/bacnet/basic/service/h_rp_a.c \
	$(BACNET_SRC_DIR)/bacnet/basic/service/h_whois.c \
	$(BACNET_SRC_DIR)/bacnet/basic/service/s_iam.c \
	$(BACNET_SRC_DIR)/bacnet/basic/service/s_rp.c \
	$(BACNET_SRC_DIR)/bacnet/basic/service/s_whois.c

# TARGET_EXT is defined in apps/Makefile as .exe or nothing
TARGET_BIN = ${TARGET}$(TARGET_EXT)

SRCS = $(SRC) $(BACNET_SRC) $(BACNET_BASIC_SRC) $(BACNET_PORT_SRC)

OBJS += ${SRCS:.c=.o}

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

