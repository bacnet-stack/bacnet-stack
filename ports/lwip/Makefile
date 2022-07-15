# Makefile to build BACnet/IP LwIP library

# Executable file name
TARGET = bip-lwip

SRC = bip.c

BACNET_DIR = ../../src
LWIP_DIR = /usr/include/lwip

CFLAGS += -I$(LWIP_DIR) -I$(BACNET_DIR)

OBJS += ${SRC:.c=.o}

all: Makefile $(TARGET)

lib: $(TARGET)

$(TARGET): $(OBJS) Makefile
	${AR} rcs $@ $(OBJS)

.c.o:
	${CC} -c ${CFLAGS} $*.c -o $@

.PHONY: depend
depend:
	rm -f .depend
	${CC} -MM ${CFLAGS} *.c >> .depend

.PHONY: clean
clean:
	-rm -f core ${TARGET} ${OBJS} $(TARGET).map

.PHONY: include
include: .depend
