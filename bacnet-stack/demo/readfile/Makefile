#Makefile to build BACnet Application for the Linux Port
CC      = gcc
BASEDIR = .
#CFLAGS  = -Wall -I.
# -g for debugging with gdb
#CFLAGS  = -Wall -I. -O2 -g
# Note: you can strip out symbols using the strip command
# to get an idea of how big the compile really is.
#DEFINES = -DBACFILE=1 -DBACDL_ETHERNET=1
#DEFINES = -DBACFILE=1 -DBACDL_ARCNET=1
#DEFINES = -DBACFILE=1 -DBACDL_MSTP=1 
DEFINES = -DBACFILE=1 -DBACDL_BIP=1
PORTS = ../../ports/linux
BACNET_OBJECT = ../object
BACNET_HANDLER = ../handler 
INCLUDES = -I. -I$(PORTS) -I$(BACNET_OBJECT) -I$(BACNET_HANDLER)

CFLAGS  = -Wall -g $(INCLUDES) $(DEFINES)

TARGET = bacarf

SRCS = readfile.c \
       $(PORTS)/bip-init.c \
       bip.c \
       $(BACNET_HANDLER)/txbuf.c \
       $(BACNET_HANDLER)/noserv.c \
       $(BACNET_HANDLER)/h_whois.c \
       $(BACNET_HANDLER)/h_rp.c \
       rp.c \
       bacdcode.c \
       bacapp.c \
       bacprop.c \
       bacstr.c \
       bactext.c \
       indtext.c \
       bigend.c \
       whois.c \
       iam.c \
       tsm.c \
       datalink.c \
       address.c \
       $(BACNET_OBJECT)/device.c \
       $(BACNET_OBJECT)/ai.c \
       $(BACNET_OBJECT)/ao.c \
       $(BACNET_OBJECT)/bacfile.c \
       arf.c \
       abort.c \
       reject.c \
       bacerror.c \
       apdu.c \
       npdu.c

OBJS = ${SRCS:.c=.o}

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

