#Makefile to build test case
CC      = gcc
SRC_DIR = ../../src
TEST_DIR = ../../test
HANDLER_DIR = ../handler
INCLUDES = -I../../include -I$(TEST_DIR) -I. -I$(HANDLER_DIR)
DEFINES = -DBIG_ENDIAN=0 -DBACDL_ALL -DBAC_TEST -DTEST_ACCUMULATOR

CFLAGS  = -Wall $(INCLUDES) $(DEFINES) -g

SRCS = acc.c \
	$(SRC_DIR)/bacdcode.c \
	$(SRC_DIR)/bacint.c \
	$(SRC_DIR)/bacstr.c \
	$(SRC_DIR)/bacreal.c \
	$(SRC_DIR)/bacapp.c \
	$(SRC_DIR)/bacdevobjpropref.c \
	$(SRC_DIR)/bactext.c \
	$(SRC_DIR)/indtext.c \
	$(SRC_DIR)/datetime.c \
	$(TEST_DIR)/ctest.c

TARGET = accumulator

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
	rm -rf core ${TARGET} $(OBJS)

include: .depend

