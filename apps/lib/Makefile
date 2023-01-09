# BACnet Stack library
# Build all the non-OS or port specific code

BACNET_LIB_NAME ?= bacnet
BACNET_LIB_DIR ?= $(realpath .)
BACNET_LIB_TARGET ?= $(BACNET_LIB_DIR)/lib$(BACNET_LIB_NAME).a
BACNET_SRC_DIR ?= $(realpath ../../src)
BACNET_PORT_DIR ?= $(realpath ../../ports/linux)
BACNET_DEFINES ?=

#build for release (default) or debug
OPTIMIZATION ?= -Os
DEBUGGING ?=
WARNINGS ?= -Wall -Wmissing-prototypes
# dead code removal
ifeq (${BUILD},debug)
OPTIMIZATION = -O0
DEBUGGING = -g -DDEBUG_ENABLED=1
endif

# put all the flags together
INCLUDES = -I$(BACNET_SRC_DIR)
CFLAGS += $(WARNINGS) $(DEBUGGING) $(OPTIMIZATION) $(BACNET_DEFINES) $(INCLUDES)
CFLAGS += -ffunction-sections -fdata-sections

PORT_ARCNET_SRC = \
	$(BACNET_PORT_DIR)/arcnet.c

PORT_MSTP_SRC = \
	$(BACNET_PORT_DIR)/rs485.c \
	$(BACNET_PORT_DIR)/dlmstp.c \
	$(BACNET_SRC_DIR)/bacnet/datalink/mstp.c \
	$(BACNET_SRC_DIR)/bacnet/datalink/mstptext.c \
	$(BACNET_SRC_DIR)/bacnet/datalink/crc.c

PORT_ETHERNET_SRC = \
	$(BACNET_PORT_DIR)/ethernet.c

PORT_BIP_SRC = \
	$(BACNET_PORT_DIR)/bip-init.c \
	$(BACNET_SRC_DIR)/bacnet/datalink/bvlc.c \
	$(BACNET_SRC_DIR)/bacnet/basic/bbmd/h_bbmd.c

PORT_BIP6_SRC = \
	$(BACNET_PORT_DIR)/bip6.c \
	$(BACNET_SRC_DIR)/bacnet/basic/bbmd6/h_bbmd6.c \
	$(BACNET_SRC_DIR)/bacnet/basic/bbmd6/vmac.c \
	$(BACNET_SRC_DIR)/bacnet/datalink/bvlc6.c

PORT_ALL_SRC = \
	$(BACNET_SRC_DIR)/bacnet/datalink/datalink.c \
	$(PORT_ARCNET_SRC) \
	$(PORT_MSTP_SRC) \
	$(PORT_ETHERNET_SRC) \
	$(PORT_BIP_SRC) \
	$(PORT_BIP6_SRC)

PORT_NONE_SRC = \
	$(BACNET_SRC_DIR)/bacnet/datalink/datalink.c

ifeq (${BACDL_DEFINE},-DBACDL_BIP=1)
BACNET_PORT_SRC = ${PORT_BIP_SRC}
endif
ifeq (${BACDL_DEFINE},-DBACDL_BIP6=1)
BACNET_PORT_SRC = ${PORT_BIP6_SRC}
endif
ifeq (${BACDL_DEFINE},-DBACDL_MSTP=1)
BACNET_PORT_SRC = ${PORT_MSTP_SRC}
endif
ifeq (${BACDL_DEFINE},-DBACDL_ARCNET=1)
BACNET_PORT_SRC = ${PORT_ARCNET_SRC}
endif
ifeq (${BACDL_DEFINE},-DBACDL_ETHERNET=1)
BACNET_PORT_SRC = ${PORT_ETHERNET_SRC}
endif
ifeq (${BACDL_DEFINE},-DBACDL_NONE=1)
BACNET_PORT_SRC = ${PORT_NONE_SRC}
endif
ifeq (${BACDL_DEFINE},-DBACDL_ALL=1)
BACNET_PORT_SRC = ${PORT_ALL_SRC}
endif
ifneq (${BACDL_DEFINE},)
CFLAGS += ${BACDL_DEFINE}
endif

BACNET_PORT_SRC += \
	$(BACNET_SRC_DIR)/bacnet/datalink/dlenv.c \
	$(BACNET_PORT_DIR)/mstimer-init.c \
	$(BACNET_PORT_DIR)/datetime-init.c

BACNET_SRC ?= \
	$(wildcard $(BACNET_SRC_DIR)/bacnet/*.c) \

BACNET_BASIC_SRC ?= \
	$(wildcard $(BACNET_SRC_DIR)/bacnet/basic/*.c) \
	$(wildcard $(BACNET_SRC_DIR)/bacnet/basic/binding/*.c) \
	$(wildcard $(BACNET_SRC_DIR)/bacnet/basic/service/*.c) \
	$(wildcard $(BACNET_SRC_DIR)/bacnet/basic/sys/*.c) \
	$(BACNET_SRC_DIR)/bacnet/basic/npdu/h_npdu.c \
	$(BACNET_SRC_DIR)/bacnet/basic/npdu/s_router.c \
	$(BACNET_SRC_DIR)/bacnet/basic/tsm/tsm.c

# build in uci integration - use UCI=1 when invoking make
ifeq (${UCI},1)
BACNET_BASIC_SRC += \
	$(wildcard $(BACNET_SRC_DIR)/bacnet/basic/ucix/*.c)
endif

SRCS := $(BACNET_SRC) $(BACNET_BASIC_SRC) $(BACNET_PORT_SRC)

OBJS = ${SRCS:.c=.o}

all: $(BACNET_LIB_TARGET)

lib: $(BACNET_LIB_TARGET)

$(BACNET_LIB_TARGET): $(OBJS) Makefile
	${AR} rcs $@ $(OBJS)

.c.o:
	${CC} -c ${CFLAGS} $*.c -o $@

depend:
	rm -f .depend
	${CC} -MM ${CFLAGS} *.c >> .depend

clean:
	rm -rf core $(OBJS) $(BACNET_LIB_TARGET)

include: .depend
