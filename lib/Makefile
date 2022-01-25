# BACnet Stack library
# Build all the non-OS or port specific code

TARGET = bacnet
LIBRARY = lib$(TARGET).a
BACNET_SRC_DIR =  $(realpath ../src)
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

BACNET_SRC ?= \
	$(wildcard $(BACNET_SRC_DIR)/bacnet/*.c) \

BACNET_BASIC_SRC ?= \
	$(wildcard $(BACNET_SRC_DIR)/bacnet/basic/*.c) \
	$(wildcard $(BACNET_SRC_DIR)/bacnet/basic/binding/*.c) \
	$(wildcard $(BACNET_SRC_DIR)/bacnet/basic/sys/*.c) \
	$(BACNET_SRC_DIR)/bacnet/basic/npdu/h_npdu.c \
	$(BACNET_SRC_DIR)/bacnet/basic/npdu/s_router.c \
	$(BACNET_SRC_DIR)/bacnet/basic/tsm/tsm.c

SRCS := $(BACNET_SRC) $(BACNET_BASIC_SRC)

OBJS = ${SRCS:.c=.o}

all: $(LIBRARY)

lib: $(LIBRARY)

$(LIBRARY): $(OBJS) Makefile
	${AR} rcs $@ $(OBJS)

.c.o:
	${CC} -c ${CFLAGS} $*.c -o $@

depend:
	rm -f .depend
	${CC} -MM ${CFLAGS} *.c >> .depend

clean:
	rm -rf core $(OBJS) $(LIBRARY)

include: .depend
