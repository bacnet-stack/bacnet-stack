#Makefile to build BACnet Application
CC      = gcc
BASEDIR = .
#CFLAGS  = -Wall -I.
# -g for debugging with gdb
#CFLAGS  = -Wall -I. -g
CFLAGS  = -Wall -I. -g

SRCS = ports/linux/main.c \
       ports/linux/ethernet.c \
       bacdcode.c \
       bigend.c \
       whois.c \
       iam.c \
       reject.c \
       apdu.c \
       npdu.c

OBJS = ${SRCS:.c=.o}

TARGET = bacnet

all: ${TARGET}
 
${TARGET}: ${OBJS}
	${CC} -o $@ ${OBJS} 

.c.o:
	${CC} -c ${CFLAGS} $*.c -o $@
	
depend:
	rm -f .depend
	${CC} -MM ${CFLAGS} *.c >> .depend
	
clean:
	rm -rf core ${TARGET} $(OBJS) *.bak ports/linux/*.bak *.1 *.ini

include: .depend

