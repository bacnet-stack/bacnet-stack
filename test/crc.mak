#Makefile to build CRC tests
CC      = gcc
SRC_DIR = ../src
INCLUDES = -I$(SRC_DIR) -I. -I../demo/object 
DEFINES = -DBIG_ENDIAN=0 -DTEST -DTEST_CRC

CFLAGS  = -Wall $(INCLUDES) $(DEFINES) -g

SRCS = $(SRC_DIR)/bacnet/datalink/crc.c \
	ctest.c

OBJS = ${SRCS:.c=.o}

TARGET = crc

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

