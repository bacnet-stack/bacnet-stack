#Makefile to build unit tests
CC = gcc
SRC_DIR = ../src
INCLUDES = -I$(SRC_DIR) -I.
DEFINES = -DBIG_ENDIAN=0 -DBAC_TEST -DTEST_KEYLIST

CFLAGS  = -Wall $(INCLUDES) $(DEFINES) -g

SRCS = $(SRC_DIR)/bacnet/basic/sys/keylist.c \
	ctest.c

TARGET_NAME = keylist
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
