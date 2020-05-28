#Makefile to build test case

# tools - only if you need them.
# Most platforms have this already defined
# CC = gcc
# AR = ar
# MAKE = make
# SIZE = size
#
# Assumes rm and cp are available

SRC_DIR := ../src
INCLUDES := -I$(SRC_DIR) -I.
DEFINES := -DBIG_ENDIAN=0 -DBAC_TEST -DBACAPP_ALL -DTEST_TIMESYNC

CFLAGS  := $(INCLUDES) $(DEFINES) -g
CFLAGS += -Wall

TARGET_NAME := timesync
ifeq ($(OS),Windows_NT)
TARGET_EXT = .exe
else
TARGET_EXT =
endif
TARGET = $(TARGET_NAME)$(TARGET_EXT)

SRCS := $(SRC_DIR)/bacnet/bacdcode.c \
	$(SRC_DIR)/bacnet/bacint.c \
	$(SRC_DIR)/bacnet/bacstr.c \
	$(SRC_DIR)/bacnet/bacreal.c \
	$(SRC_DIR)/bacnet/bacerror.c \
	$(SRC_DIR)/bacnet/bacapp.c \
	$(SRC_DIR)/bacnet/bacdevobjpropref.c \
	$(SRC_DIR)/bacnet/bactext.c \
	$(SRC_DIR)/bacnet/indtext.c \
	$(SRC_DIR)/bacnet/datetime.c \
	$(SRC_DIR)/bacnet/lighting.c \
	$(SRC_DIR)/bacnet/timesync.c \
	ctest.c

OBJS := ${SRCS:.c=.o}

all: ${TARGET}

${TARGET}: ${OBJS}
	${CC} -o $@ ${OBJS}

.c.o:
	${CC} -c ${CFLAGS} $*.c -o $@

depend:
	rm -f .depend
	${CC} -MM ${CFLAGS} *.c >> .depend

clean:
	rm -rf core ${TARGET} $(OBJS) *.bak *.1 *.ini

run:
	./${TARGET}

include: .depend

.PHONY: all run clean
