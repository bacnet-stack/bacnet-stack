#Makefile to build test case
CC      = gcc
SRC_DIR = ../../src
TEST_DIR = ../../test
INCLUDES = -I../../include -I$(TEST_DIR) -I.
DEFINES = -DBIG_ENDIAN=0 -DTEST -DBACAPP_ALL -DTEST_CREDENTIAL_DATA_INPUT

CFLAGS  = -Wall $(INCLUDES) $(DEFINES) -g

SRCS = credential_data_input.c \
	$(SRC_DIR)/bacdcode.c \
	$(SRC_DIR)/bacint.c \
	$(SRC_DIR)/bacstr.c \
	$(SRC_DIR)/bacreal.c \
	$(SRC_DIR)/datetime.c \
	$(SRC_DIR)/lighting.c \
	$(SRC_DIR)/bacapp.c \
	$(SRC_DIR)/bacdevobjpropref.c \
	$(SRC_DIR)/bactext.c \
	$(SRC_DIR)/indtext.c \
	$(SRC_DIR)/authentication_factor.c \
	$(SRC_DIR)/authentication_factor_format.c \
	$(SRC_DIR)/timestamp.c \
	$(TEST_DIR)/ctest.c

TARGET = credential_data_input

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
