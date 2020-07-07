#Makefile to build test case
CC      = gcc
SRC_DIR = ../../src
TEST_DIR = ../../test
INCLUDES = -I../../include -I$(TEST_DIR) -I.
DEFINES = -DBIG_ENDIAN=0 -DBAC_TEST -DBACAPP_ALL -DTEST_CREDENTIAL_DATA_INPUT

CFLAGS  = -Wall $(INCLUDES) $(DEFINES) -g

SRCS = credential_data_input.c \
	$(SRC_DIR)/bacnet/bacdcode.c \
	$(SRC_DIR)/bacnet/bacint.c \
	$(SRC_DIR)/bacnet/bacstr.c \
	$(SRC_DIR)/bacnet/bacreal.c \
	$(SRC_DIR)/bacnet/datetime.c \
	$(SRC_DIR)/bacnet/lighting.c \
	$(SRC_DIR)/bacnet/bacapp.c \
	$(SRC_DIR)/bacnet/bacdevobjpropref.c \
	$(SRC_DIR)/bacnet/bactext.c \
	$(SRC_DIR)/bacnet/indtext.c \
	$(SRC_DIR)/bacnet/authentication_factor.c \
	$(SRC_DIR)/bacnet/authentication_factor_format.c \
	$(SRC_DIR)/bacnet/timestamp.c \
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
