#Makefile to build BACnet Application for the Linux Port
CC      = gcc
BASEDIR = .
#CFLAGS  = -Wall -I.
# -g for debugging with gdb
#CFLAGS  = -Wall -I. -O2 -g
# Note: you can strip out symbols using the strip command
# to get an idea of how big the compile really is.
#CFLAGS  = -Wall -I. -Iports/linux -g -DBACFILE=1 -DBACDL_ETHERNET=1
#CFLAGS  = -Wall -I. -Iports/linux -g -DBACFILE=1 -DBACDL_ARCNET=1
#CFLAGS  = -Wall -I. -Iports/linux -g -DBACFILE=1 -DBACDL_MSTP=1 
CFLAGS  = -Wall -I. -Iports/linux -g -DBACFILE=1 -DBACDL_BIP=1

SRCS = ports/linux/main.c \
       ports/linux/ethernet.c \
       ports/linux/arcnet.c \
       ports/linux/bip-init.c \
       bip.c \
       dlmstp.c \
       handlers.c \
       bacdcode.c \
       bigend.c \
       whois.c \
       iam.c \
       rp.c \
       rpm.c \
       wp.c \
       tsm.c \
       datalink.c \
       address.c \
       device.c \
       ai.c \
       ao.c \
       bacfile.c \
       arf.c \
       awf.c \
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

