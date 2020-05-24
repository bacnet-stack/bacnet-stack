#Makefile to build test case
CC      = gcc
SRC_DIR = ../../../src
TEST_DIR = ../../../test
INCLUDES = -I$(SRC_DIR) -I$(TEST_DIR)
DEFINES = -DDEBUG_ENABLED=0

CFLAGS  = -Wall $(INCLUDES) $(DEFINES) -g

SRCS = main.c \
	$(SRC_DIR)/bacnet/bacreal.c \
	$(SRC_DIR)/bacnet/basic/sys/bigend.c \
	$(TEST_DIR)/ctest.c

TARGET_NAME = unittest
ifeq ($(OS),Windows_NT)
TARGET_EXT = .exe
else
TARGET_EXT =
endif
TARGET = $(TARGET_NAME)$(TARGET_EXT)

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
	rm -rf ${TARGET} $(OBJS)

test: ${TARGET}
	./${TARGET}

include: .depend
