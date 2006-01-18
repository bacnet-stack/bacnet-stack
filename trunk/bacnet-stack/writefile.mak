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
INCLUDES = -I. -Iports/linux -Idemo/object -Idemo/handler

CFLAGS  = -Wall -g $(INCLUDES) $(DEFINES)

TARGET = bacawf

SRCS = demo/writefile/writefile.c \
       ports/linux/bip-init.c \
       bip.c \
       demo/handler/txbuf.c \
       demo/handler/noserv.c \
       demo/handler/h_whois.c \
       demo/handler/h_rp.c \
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
       demo/object/device.c \
       demo/object/ai.c \
       demo/object/ao.c \
       demo/object/bacfile.c \
       arf.c \
       awf.c \
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

