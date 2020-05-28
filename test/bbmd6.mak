#Makefile to build test case
CC      = gcc
SRC_DIR = ../src
SRC_INC = ../include
DEMO_DIR = ../demo/handler
DEMO_INC = ../demo/object
INCLUDES =  -I. -I$(SRC_INC) -I$(DEMO_INC)
DEFINES = -DBIG_ENDIAN=0 -DBAC_TEST -DTEST_BBMD6

CFLAGS  = -Wall -Wmissing-prototypes $(INCLUDES) $(DEFINES) -g

SRCS = $(SRC_DIR)/bacnet/bacdcode.c \
	$(SRC_DIR)/bacnet/bacint.c \
	$(SRC_DIR)/bacnet/bacstr.c \
	$(SRC_DIR)/bacnet/bacreal.c \
	$(SRC_DIR)/bacnet/bvlc6.c \
	$(SRC_DIR)/bacnet/debug.c \
	$(SRC_DIR)/bacnet/basic/sys/keylist.c \
	$(SRC_DIR)/bacnet/basic/bbmd6/vmac.c \
	$(DEMO_DIR)/h_bbmd6.c \
	ctest.c

TARGET_NAME = bbmd6
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

include: .depend
