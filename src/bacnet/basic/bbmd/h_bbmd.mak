#Makefile to build test case
CC      = gcc
SRC_DIR = ../../../../src
TEST_DIR = ../../../../test
INCLUDES = -I$(SRC_DIR) -I$(TEST_DIR)
DEFINES = -DBIG_ENDIAN=0 -DBACDL_BIP -DBBMD_ENABLED=1 -DTEST -DTEST_BBMD_HANDLER

CFLAGS  = -Wall $(INCLUDES) $(DEFINES) -g

SRCS = $(SRC_DIR)/bacnet/basic/bbmd/h_bbmd.c \
	$(SRC_DIR)/bacnet/bacdcode.c \
	$(SRC_DIR)/bacnet/bacint.c \
	$(SRC_DIR)/bacnet/bacstr.c \
	$(SRC_DIR)/bacnet/bacreal.c \
	$(SRC_DIR)/bacnet/datalink/bvlc.c \
	$(SRC_DIR)/bacnet/basic/sys/debug.c \
	$(TEST_DIR)/ctest.c

TARGET_NAME = bbmd
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
	rm -rf core ${TARGET} $(OBJS) *.bak *.1 *.ini

test: ${TARGET}
	./${TARGET}

include: .depend
