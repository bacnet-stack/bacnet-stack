#Makefile to build unit tests
CC      = gcc
SRC_DIR = ../src
INCLUDES = -I$(SRC_DIR) -I.
DEFINES = -DBIG_ENDIAN=0 -DBAC_TEST -DTEST_BACSTR

CFLAGS  = -Wall $(INCLUDES) $(DEFINES) -g

TARGET_NAME = bacstr
ifeq ($(OS),Windows_NT)
TARGET_EXT = .exe
else
TARGET_EXT =
endif
TARGET = $(TARGET_NAME)$(TARGET_EXT)

SRCS = $(SRC_DIR)/bacnet/bacstr.c \
	ctest.c

OBJS = ${SRCS:.c=.o}

all: ${TARGET}

${TARGET}: ${OBJS}
	${CC} -o $@ ${OBJS}

.c.o:
	${CC} -c ${CFLAGS} $*.c -o $@

depend:
	rm -f .depend
	${CC} -MM ${CFLAGS} *.c >> .depend

clean:
	rm -rf core ${OBJS} ${TARGET} *.bak

include: .depend
