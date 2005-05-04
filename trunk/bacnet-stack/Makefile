#Makefile to build BACnet Application
CC      = gcc
BASEDIR = .
#CFLAGS  = -Wall -I.
# -g for debugging with gdb
#CFLAGS  = -Wall -I. -O2 -g
# Note: you can strip out symbols using the strip command
# to get an idea of how big the compile really is.
CFLAGS  = -Wall -I. -g

SRCS = ports/linux/main.c \
       ports/linux/ethernet.c \
       handlers.c \
       bacdcode.c \
       bigend.c \
       whois.c \
       iam.c \
       rp.c \
       wp.c \
       device.c \
       ai.c \
       abort.c \
       reject.c \
       bacerror.c \
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

