#Makefile to build test case
CC      = gcc
SRC_DIR = ../src
SRC_INC = ../include
DEMO_DIR = ../demo/handler
DEMO_INC = ../demo/object
INCLUDES =  -I. -I$(SRC_INC) -I$(DEMO_INC)
DEFINES = -DBIG_ENDIAN=0 -DTEST -DTEST_BBMD6

CFLAGS  = -Wall -Wmissing-prototypes $(INCLUDES) $(DEFINES) -g

SRCS = $(SRC_DIR)/bacdcode.c \
	$(SRC_DIR)/bacint.c \
	$(SRC_DIR)/bacstr.c \
	$(SRC_DIR)/bacreal.c \
	$(SRC_DIR)/bvlc6.c \
	$(SRC_DIR)/debug.c \
	$(SRC_DIR)/keylist.c \
	$(SRC_DIR)/vmac.c \
	$(DEMO_DIR)/h_bbmd6.c \
	ctest.c

TARGET = bbmd6

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
