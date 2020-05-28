#Makefile to build CRC tests
CC      = gcc
SRC_DIR = ../src
INCLUDES = -I$(SRC_DIR) -I. -I../demo/object
DEFINES = -DBIG_ENDIAN=0 -DBAC_TEST -DTEST_CRC

CFLAGS  = -Wall $(INCLUDES) $(DEFINES) -g

SRCS = $(SRC_DIR)/bacnet/datalink/crc.c \
	ctest.c

OBJS = ${SRCS:.c=.o}

TARGET_NAME = crc
ifeq ($(OS),Windows_NT)
TARGET_EXT = .exe
else
TARGET_EXT =
endif
TARGET = $(TARGET_NAME)$(TARGET_EXT)

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

include: .depend

