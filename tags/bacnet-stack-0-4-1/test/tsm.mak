#Makefile to build test case
CC      = gcc
SRC_DIR = ../src
DEMO_OBJECT_DIR = ../demo/object
INCLUDES = -I../include -I.
DEFINES = -DBIG_ENDIAN=0 -DTEST -DBACAPP_ALL -DBACDL_TEST -DTEST_TSM

CFLAGS  = -Wall $(INCLUDES) $(DEFINES) -g

SRCS = $(SRC_DIR)/bacdcode.c \
	$(SRC_DIR)/bacint.c \
	$(SRC_DIR)/bacstr.c \
	$(SRC_DIR)/bacreal.c \
	$(SRC_DIR)/bacerror.c \
	$(SRC_DIR)/bacapp.c \
	$(SRC_DIR)/bacaddr.c \
	$(SRC_DIR)/bactext.c \
	$(SRC_DIR)/indtext.c \
	$(SRC_DIR)/datetime.c \
	$(SRC_DIR)/iam.c \
	$(SRC_DIR)/dcc.c \
	$(SRC_DIR)/npdu.c \
	$(SRC_DIR)/apdu.c \
	$(SRC_DIR)/version.c \
	$(SRC_DIR)/tsm.c \
	$(DEMO_OBJECT_DIR)/device.c \
	$(DEMO_OBJECT_DIR)/ai.c \
	$(DEMO_OBJECT_DIR)/ao.c \
	$(DEMO_OBJECT_DIR)/av.c \
	$(DEMO_OBJECT_DIR)/bi.c \
	$(DEMO_OBJECT_DIR)/bo.c \
	$(DEMO_OBJECT_DIR)/bv.c \
	$(DEMO_OBJECT_DIR)/lsp.c \
	$(DEMO_OBJECT_DIR)/mso.c \
	$(DEMO_OBJECT_DIR)/lc.c \
	ctest.c

TARGET = tsm

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

include: .depend
