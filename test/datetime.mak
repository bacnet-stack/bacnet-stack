#Makefile to build test case
CC = gcc
SRC_DIR = ../src
INCLUDES = -I$(SRC_DIR) -I.
DEFINES = -DBIG_ENDIAN=0 -DBAC_TEST -DTEST_DATE_TIME

CFLAGS  = -Wall $(INCLUDES) $(DEFINES) -g

SRCS = $(SRC_DIR)/bacnet/datetime.c \
	$(SRC_DIR)/bacnet/bacdcode.c \
	$(SRC_DIR)/bacnet/bacint.c \
	$(SRC_DIR)/bacnet/bacstr.c \
	$(SRC_DIR)/bacnet/bacreal.c \
	$(SRC_DIR)/bacnet/bacapp.c \
	$(SRC_DIR)/bacnet/bacdevobjpropref.c \
	$(SRC_DIR)/bacnet/lighting.c \
	$(SRC_DIR)/bacnet/bactext.c \
	$(SRC_DIR)/bacnet/indtext.c \
	ctest.c

OBJS = ${SRCS:.c=.o}

TARGET_NAME = datetime
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
