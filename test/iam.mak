#Makefile to build test case
CC = gcc
SRC_DIR = ../src
INCLUDES = -I../include -I.
DEFINES = -DBIG_ENDIAN=0 -DTEST -DTEST_IAM -DBACDL_TEST

CFLAGS  = -Wall $(INCLUDES) $(DEFINES) -g

SRCS = $(SRC_DIR)/bacdcode.c \
	$(SRC_DIR)/bacint.c \
	$(SRC_DIR)/bacstr.c \
	$(SRC_DIR)/bacreal.c \
	$(SRC_DIR)/npdu.c \
	$(SRC_DIR)/apdu.c \
	$(SRC_DIR)/dcc.c \
	$(SRC_DIR)/iam.c \
	ctest.c

OBJS = ${SRCS:.c=.o}

TARGET = iam

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
