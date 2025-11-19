#Makefile to build BACnet Library with GCC

# tools - only if you need them.
# Most platforms have this already defined
# CC = gcc
# AR = ar
# MAKE = make
# SIZE = size
#
# Assumes rm and cp are available

CFLAGS += -DBACNET_STACK_EXPORTS

ifeq ($(STATIC),1)
	CFLAGS += -DBACNET_STACK_STATIC_DEFINE
endif

# These might be already defined in an previous Makefile
BACNET_PORT ?= linux
BACNET_SRC_DIR ?= $(realpath .)
BACNET_PORT_DIR ?= $(realpath ../ports/$(BACNET_PORT))
BACNET_PORT_SRC ?= \
	$(BACNET_PORT_DIR)/bip-init.c \
	$(BACNET_SRC_DIR)/bacnet/datalink/bvlc.c \
	$(BACNET_SRC_DIR)/bacnet/basic/bbmd/h_bbmd.c

# include file search paths
BACNET_INCLUDES = -I$(BACNET_SRC_DIR) -I$(BACNET_PORT_DIR)
CFLAGS += $(BACNET_INCLUDES)

BACNET_SRC ?= \
	$(wildcard $(BACNET_SRC_DIR)/bacnet/*.c) \
	$(wildcard $(BACNET_SRC_DIR)/bacnet/basic/*.c) \
	$(wildcard $(BACNET_SRC_DIR)/bacnet/basic/binding/*.c) \
	$(wildcard $(BACNET_SRC_DIR)/bacnet/basic/service/*.c) \
	$(wildcard $(BACNET_SRC_DIR)/bacnet/basic/npdu/*.c) \
	$(wildcard $(BACNET_SRC_DIR)/bacnet/basic/sys/*.c) \
	$(wildcard $(BACNET_SRC_DIR)/bacnet/basic/tsm/*.c)

ifneq (,$(findstring -DBAC_UCI,$(BACNET_DEFINES)))
OBJS += $(patsubst %.c,%.o,$(wildcard basic/ucix/*.c))
endif

all: $(OBJS) Makefile

.c.o:
	${CC} -c ${CFLAGS} $*.c -o $@

depend:
	rm -f .depend
	${CC} -MM ${CFLAGS} $(BACNET_SRC) >> .depend

clean:
	rm -rf core $(OBJS) $(LIBRARY)

include: .depend
