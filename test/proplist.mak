#Makefile to build unit tests
CC = gcc
SRC_DIR = ../src
INCLUDES = -I$(SRC_DIR) -I.
DEFINES = -DBIG_ENDIAN=0 -DBAC_TEST -DTEST_PROPLIST
DEFINES += -DBACNET_PROPERTY_LISTS=1

CFLAGS  = -Wall $(INCLUDES) $(DEFINES) -g

SRCS = $(SRC_DIR)/bacnet/proplist.c \
	$(SRC_DIR)/bacnet/property.c \
	$(SRC_DIR)/bacnet/bacdcode.c \
	$(SRC_DIR)/bacnet/bacint.c \
	$(SRC_DIR)/bacnet/bacreal.c \
	$(SRC_DIR)/bacnet/bacstr.c \
	$(SRC_DIR)/bacnet/datetime.c \
	$(SRC_DIR)/bacnet/lighting.c \
	ctest.c

TARGET_NAME = proplist
ifeq ($(OS),Windows_NT)
TARGET_EXT = .exe
else
TARGET_EXT =
endif
TARGET = $(TARGET_NAME)$(TARGET_EXT)

OBJS  = ${SRCS:.c=.o}

all: ${TARGET}

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
