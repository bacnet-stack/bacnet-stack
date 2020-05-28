#Makefile to build test case
CC      = gcc
SRC_DIR = ../../src
TEST_DIR = ../../test
PORTS_DIR = ../../ports/linux
INCLUDES = -I../../include -I$(TEST_DIR) -I$(PORTS_DIR) -I.
DEFINES = -DBIG_ENDIAN=0
DEFINES += -DBAC_TEST -DBACDL_TEST
DEFINES += -DBACAPP_ALL
DEFINES += -DMAX_TSM_TRANSACTIONS=0
DEFINES += -DTEST_DEVICE
DEFINES += -DBACNET_PROPERTY_LISTS=1

CFLAGS  = -Wall $(INCLUDES) $(DEFINES) -g

SRCS = device.c \
	$(SRC_DIR)/bacnet/bacdcode.c \
	$(SRC_DIR)/bacnet/bacint.c \
	$(SRC_DIR)/bacnet/bacstr.c \
	$(SRC_DIR)/bacnet/bacreal.c \
	$(SRC_DIR)/bacnet/datetime.c \
	$(SRC_DIR)/bacnet/bacapp.c \
	$(SRC_DIR)/bacnet/bacdevobjpropref.c \
	$(SRC_DIR)/bacnet/bactext.c \
	$(SRC_DIR)/bacnet/indtext.c \
	$(SRC_DIR)/bacnet/proplist.c \
	$(SRC_DIR)/bacnet/lighting.c \
	$(SRC_DIR)/bacnet/basic/service/h_apdu.c \
	$(SRC_DIR)/bacnet/address.c \
	$(SRC_DIR)/bacnet/bacaddr.c \
	$(SRC_DIR)/bacnet/dcc.c \
	$(SRC_DIR)/bacnet/version.c \
	$(TEST_DIR)/ctest.c

TARGET = device

all: ${TARGET}

OBJS = ${SRCS:.c=.o}

${TARGET}: ${OBJS}
	${CC} -o $@ ${OBJS}

.c.o:
	${CC} -c ${CFLAGS} $*.c -o $@

depend:
	rm -f .depend
	${CC} -MM ${CFLAGS} *.c >> .depend

clean:
	rm -rf core ${TARGET} $(OBJS)

include: .depend
